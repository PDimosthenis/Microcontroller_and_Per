#include "bmp280.h"
#include "delay.h"

extern void i2c_init(void);
extern void i2c_write(uint8_t address, uint8_t *buffer, int buff_len);
extern void i2c_read(uint8_t address, uint8_t *buffer, int buff_len);

static BMP280_CalibData calib;
static int32_t t_fine;
static uint8_t current_filter = (BMP280_FILTER_X16 << 2); 
static uint8_t filter_on = 1;

// Temp var to hold the ctrl_meas register and later  mask whatever we want 
static uint8_t shadow_ctrl_meas = 0; 

//Give the oversampling settings and mode that you want and will return an 8bit  to be written into the sensors ctrl_mes reg
uint8_t BMP280_Build_CtrlMeas(BMP280_Oversampling_t osrs_t, BMP280_Oversampling_t osrs_p, BMP280_Mode_t mode) {
    return (uint8_t)((osrs_t << 5) | (osrs_p << 2) | mode);
}

//Give the standby time, Filter setting and 0 for spi and will return an 8bit to be written in sensors cinfig reg
uint8_t BMP280_Build_Config(BMP280_StandbyTime_t t_sb, BMP280_Filter_t filter, uint8_t spi3w_en) {
    return (uint8_t)((t_sb << 5) | (filter << 2) | (spi3w_en & 0x01));
}

//Give the addr of reg you want to read from to and a buff to store what you read
void BMP280_ReadRegs(uint8_t reg, uint8_t *data, int len) {
    i2c_write(BMP280_I2C_ADDR, &reg, 1); //Based on datasheet first send on write mode the reg address
    i2c_read(BMP280_I2C_ADDR, data, len); //Burst read the registers you want
}

//Give the addr of the reg you want to write to and the value to be written
void BMP280_WriteReg(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val}; //Create a buffer cause based on datasheet we continiously send the addr and then the data
    i2c_write(BMP280_I2C_ADDR, buf, 2); //Write
}

void BMP280_Init(void) {
    i2c_init(); //Initalize the i2c
    
    //Software reset the sensor because it has power on reset but if we just load another code without power on off dosent reset
    BMP280_WriteReg(RESET_ADDR, RESET_MSG);
    //The reset is based on power on reset and since startup time 2ms lets wait 4 just for safety
    delay_ms(4);
    
    uint8_t buf[24];
    //Read the Calib data from the sensor
    BMP280_ReadRegs(0x88, buf, 24);
    //Create each of the compensation params from the data we just read
    calib.dig_T1 = (buf[1] << 8) | buf[0];
    calib.dig_T2 = (buf[3] << 8) | buf[2];
    calib.dig_T3 = (buf[5] << 8) | buf[4];
    calib.dig_P1 = (buf[7] << 8) | buf[6];
    calib.dig_P2 = (buf[9] << 8) | buf[8];
    calib.dig_P3 = (buf[11] << 8) | buf[10];
    calib.dig_P4 = (buf[13] << 8) | buf[12];
    calib.dig_P5 = (buf[15] << 8) | buf[14];
    calib.dig_P6 = (buf[17] << 8) | buf[16];
    calib.dig_P7 = (buf[19] << 8) | buf[18];
    calib.dig_P8 = (buf[21] << 8) | buf[20];
    calib.dig_P9 = (buf[23] << 8) | buf[22];
    
    //Initalizing the standby time at 0.5ms the filter setting at x16 and lsb 0 since we have i2c      
    BMP280_WriteReg(0xF5, BMP280_Build_Config(BMP280_STANDBY_0_5_MS, BMP280_FILTER_X16, 0x00)); 
    
    // Setting the oversample settings for temr, pressure and mode
    shadow_ctrl_meas = BMP280_Build_CtrlMeas(BMP280_OVERSAMP_X2, BMP280_OVERSAMP_X4, BMP280_MODE_NORMAL);
    BMP280_WriteReg(0xF4, shadow_ctrl_meas);  
}

static int32_t bmp280_compensate_T_int32(int32_t adc_T) {
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) * ((int32_t)calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

static uint32_t bmp280_compensate_P_int64(int32_t adc_P) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;
    
    if (var1 == 0) return 0; 

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (uint32_t)p;
}

void BMP280_Read_Temp_Press(float *temperature, float *pressure) {
    uint8_t data[6];

    BMP280_ReadRegs(0xF7, data, 6);

    int32_t adc_P = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    int32_t adc_T = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);

    int32_t temp_raw = bmp280_compensate_T_int32(adc_T);
    uint32_t press_raw = bmp280_compensate_P_int64(adc_P);

    *temperature = temp_raw / 100.0f;
    // Divide by 100 to take hpa and not pa
    *pressure = (press_raw / 256.0f) / 100.0f; 
}
 
void BMP280_change_mode(BMP280_Mode_t mode){
    // and it with 11111100 to clear the mode bits and or the new mode
    shadow_ctrl_meas = (shadow_ctrl_meas & 0xFC) | mode; 
    BMP280_WriteReg(0xF4, shadow_ctrl_meas); 
}

void BMP280_filter_disable(){
    uint8_t config;
    BMP280_ReadRegs(0xF5, &config, 1); 
    current_filter = config & 0x1C;  
    config = (config & 0xE3); 
    BMP280_WriteReg(0xF5, config); 
}

void BMP280_filter_enable(){
    uint8_t config;
    BMP280_ReadRegs(0xF5, &config, 1);
    config = (config & 0xE3); 
    config = (config | current_filter); 
    BMP280_WriteReg(0xF5, config); 
}

void BMP280_filter_TOGGLE(){
    if(filter_on){
        BMP280_filter_disable();
        filter_on = 0;
    }
    else {
        BMP280_filter_enable();
        filter_on = 1;
    }
}
//Probably not nessecery since its gonna be fixed
void BMP280_sb_t_set(BMP280_StandbyTime_t sbt){
    uint8_t config;
    BMP280_ReadRegs(0xF5, &config, 1);
    config = (config & 0x1F);
    sbt = sbt << 5;
    config = (config | sbt);
    BMP280_WriteReg(0xF5, config); 
}