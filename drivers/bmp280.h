#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>
#include "delay.h"


#define BMP280_I2C_ADDR 0xEC
#define RESET_ADDR  0xE0
#define RESET_MSG  0xB6

// oversampling settings
typedef enum {
    BMP280_OVERSAMP_SKIPPED = 0x00, // 000
    BMP280_OVERSAMP_X1      = 0x01, // 001
    BMP280_OVERSAMP_X2      = 0x02, // 010
    BMP280_OVERSAMP_X4      = 0x03, // 011 (Standard resolution)
    BMP280_OVERSAMP_X8      = 0x04, // 100
    BMP280_OVERSAMP_X16     = 0x05  // 101
} BMP280_Oversampling_t;

//Power Modes)
typedef enum {
    BMP280_MODE_SLEEP  = 0x00, // 00
    BMP280_MODE_FORCED = 0x01, // 01
    BMP280_MODE_NORMAL = 0x03  // 11
} BMP280_Mode_t;

//(Standby Time)
typedef enum {
    BMP280_STANDBY_0_5_MS   = 0x00, // 000
    BMP280_STANDBY_62_5_MS  = 0x01, // 001
    BMP280_STANDBY_125_MS   = 0x02, // 010
    BMP280_STANDBY_250_MS   = 0x03, // 011
    BMP280_STANDBY_500_MS   = 0x04, // 100
    BMP280_STANDBY_1000_MS  = 0x05, // 101
    BMP280_STANDBY_2000_MS  = 0x06, // 110
    BMP280_STANDBY_4000_MS  = 0x07  // 111
} BMP280_StandbyTime_t;

// IIR filter coef settings
typedef enum {
    BMP280_FILTER_OFF = 0x00, // 000
    BMP280_FILTER_X2  = 0x01, // 001
    BMP280_FILTER_X4  = 0x02, // 010
    BMP280_FILTER_X8  = 0x03, // 011
    BMP280_FILTER_X16 = 0x04  // 100
} BMP280_Filter_t;

//Triming parameters
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibData;

// Functions
void BMP280_change_mode(BMP280_Mode_t mode);
void BMP280_Init(void);
void BMP280_Read_Temp_Press(float *temperature, float *pressure);
void BMP280_filter_disable();
void BMP280_filter_enable();
void BMP280_filter_TOGGLE();
void BMP280_sb_t_set(BMP280_StandbyTime_t sbt);
void BMP280_ReadRegs(uint8_t reg, uint8_t *data, int len);
#endif