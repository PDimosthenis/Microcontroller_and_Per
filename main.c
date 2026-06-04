/* ============================================================================
 * HARDWARE CONNECTIONS
 * ============================================================================
 * 1. BMP280 Sensor (I2C)
 * - VCC -> 3.3V (Nucleo)
 * - GND -> GND (Nucleo)
 * - SCL -> PB8
 * - SDA -> PB9
 * - SDO -> GND  (Sets I2C address to 0xEC)
 * - CSB -> 3.3V (Enables I2C mode, disables SPI)
 * * 2. Outer Button (Mode Change)
 * - Pin 1 -> 3.3V
 * - Pin 2 (Diagonal) -> PA_9 (D8)
 * * 3. External LED
 * - Long Leg (Anode +)   -> PC_7 (D9)
 * - Short Leg (Cathode -)-> Resistor (220-330 ohm) -> GND
 * * 4. On-board Button (P0 Calibration)
 * - PC_13 (Pre-wired on Nucleo board)
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "platform.h"
#include "i2c.h"
#include "bmp280.h"
#include "uart.h"
#include "gpio.h"
#include "queue.h"
#include "timer.h"
#include "delay.h"

#define QUEUE_SIZE 10
Queue rx_queue;

//Circural buffer with timestamps
typedef struct {
    float pressure;
    uint32_t timestamp;
} pressure_record_t;

#define HISTORY_SIZE 10
pressure_record_t pressure_history[HISTORY_SIZE];
uint32_t queue_head = 0;
uint32_t queue_tail = 0;
uint32_t queue_count = 0;

void update_pressure_history(float new_pressure, uint32_t current_time) {
    pressure_history[queue_tail].pressure = new_pressure;
    pressure_history[queue_tail].timestamp = current_time;
    
    queue_tail = (queue_tail + 1) % HISTORY_SIZE;
    
    if (queue_count < HISTORY_SIZE) {
        queue_count++;
    } else {
        queue_head = (queue_head + 1) % HISTORY_SIZE;
    }
}

// Only for debug to see if the filter is on or off
uint8_t filter_debug = 0;

// Reference pressure for altitude calculation (Initial Standard Value)
volatile float P_0 = 1013.25f;

// Global timer counter updated every 50ms by timer ISR
volatile uint32_t cnt_50ms = 0; 

// Interrupt flags
volatile bool outer_btn_flag = 0;
volatile bool board_btn_flag = 0;

// Filter flag
volatile bool filter_off = 1; // Initialize filter as on since that's what we do in bmp280_init() 
// Alarm flag
volatile bool alarm_flag = 0;

// Global sensor variables
float temperature = 0.0f;
float pressure = 0.0f;
float altitude = 0.0f;

// Interrupt Service Routines (ISRs)
void timer_isr() { cnt_50ms++; }  // TIMER
void outer_btn_isr() { outer_btn_flag = 1; } // OUTER_BUTTON
void board_btn_isr() { board_btn_flag = 1; } // BOARD_BUTTON
void uart_rx_isr(uint8_t rx) {              // UART_INTERRUPT
    if(rx == 'c' || rx == 's' || rx == 'f' || rx == 'C' || rx == 'S' || rx == 'F') {
        queue_enqueue(&rx_queue, rx);
    }
}

// Height Calculate function
void calc_h(float p_current) {
    altitude = 44330.0f * (1.0f - pow((p_current / P_0), (1.0f / 5.255f)));
}

// Check Alarm Conditions (Temp > 35, Drop > 10hPa from P0, Drop > 5hPa in 10s)
void check_alarm_conditions() {
    bool rate_drop_alarm = 0;

    if (queue_count >= 2) { 
        // Scan circular buffer backwards (newest to oldest)
        for (uint32_t i = 1; i <= queue_count; i++) {
            uint32_t idx = (queue_tail - i + HISTORY_SIZE) % HISTORY_SIZE;
            
            // Calculate elapsed time since this measurement
            uint32_t time_diff = cnt_50ms - pressure_history[idx].timestamp;
            
            // If measurement is within the last 10 seconds (200 * 50ms)
            if (time_diff <= 200) { 
                if ((pressure_history[idx].pressure - pressure) > 5.0f) {
                    rate_drop_alarm = 1;
                    break; // Sharp drop found, no need to search further
                }
            } else {
                // If measurement is older than 10s, stop searching
                break;
            }
        }
    }

    // Trigger alarm if any condition is met
    if ((temperature > 35.0f) ||           
        ((P_0 - pressure) > 10.0f) ||      
        (rate_drop_alarm)) {               
        
        alarm_flag = 1;
        gpio_set(PC_7, 1); // Set the OUTER LED
				gpio_set(P_LED_R, 0); //Disable boards led	
        uart_print("\r\n [ALERT] Extreme Conditions Detected!\r\n");
    }
}

// Custom type to identify the system state
typedef enum {
    NORMAL = 0,
    FORCED = 1
} state_t;

int main() {
    uint8_t rx_char; // One byte to store uart input       
    state_t mode_flag = NORMAL; // Initialize at normal
    
    // Variables for read/update and blink timings
    uint32_t normal_blink_tmr = 0;
    uint32_t normal_update_tmr = 0;
    uint32_t forced_blink_tmr = 0;
    uint32_t forced_update_tmr = 0;
    
    // OUTER_BUTTON Initialization
    gpio_set_mode(PA_9, PullDown);
    gpio_set_trigger(PA_9, Rising);
    gpio_set_callback(PA_9, outer_btn_isr);
    
    // LED Initialization
    gpio_set_mode(PC_7, Output);
	  
	  //ON BOARD LED Initialization
	  gpio_set_mode(P_LED_R, Output);
    
    // BOARD_BUTTON Initialization
    gpio_set_mode(PC_13, PullDown);
    gpio_set_trigger(PC_13, Rising);
    gpio_set_callback(PC_13, board_btn_isr);
    
    // TIMER Initialization
    timer_init(50000); 
    timer_set_callback(timer_isr);
    timer_enable();
        
    // QUEUE Initialization FOR Uart
    queue_init(&rx_queue, QUEUE_SIZE);
        
    // Uart Initialization
    uart_init(115200);
    uart_set_rx_callback(uart_rx_isr);
    uart_enable();
    
    // Setting Priorities
    NVIC_SetPriority(EXTI9_5_IRQn, 0);   // Outer btn (Highest) -> MODE SELECT
    NVIC_SetPriority(EXTI15_10_IRQn, 1); // Board btn -> Reference Pressure
    NVIC_SetPriority(SysTick_IRQn, 2);   // Timer    
    NVIC_SetPriority(USART2_IRQn, 3);    // UART (Lowest)
    
    __enable_irq(); 
    
    BMP280_Init();
        
    // Delay for Sensor Power-On
    // BMP280 Init resets the sensor, wait to allow it to initialize
    uint32_t boot_wait = cnt_50ms;
    while((cnt_50ms - boot_wait) < 4) { 
        __NOP(); 
    }
    
    uart_print("\r\nSystem Initialized at NORMAL_MODE\r\n");
    BMP280_ReadRegs(0xF5, &filter_debug, 1); // JUST FOR DEBUG
    
    // Set STANDBY TIME AT 0.5 ms based on table 15 (indoor navigation settings)
    BMP280_sb_t_set(BMP280_STANDBY_0_5_MS);
    
    // Initialize normal timers    
    normal_blink_tmr = cnt_50ms;
    normal_update_tmr = cnt_50ms;
    
    while(1) {
        
        if(alarm_flag) {
            // If uart queue is not empty, check if we received 'c' to drop the alarm
            if(queue_dequeue(&rx_queue, &rx_char)) {
                if(rx_char == 'c') {
                    alarm_flag = 0;
                    uart_print("Alarm stopped\r\n");
                    mode_flag = NORMAL; // Return to Normal function
									  gpio_set(PC_7, 0); //Close outter led
									  gpio_set(P_LED_R, 1); //Start blinking with led on
                }
            }
            // Continuously print msg while in alarm mode
            uart_print("\r\n [ALERT] Extreme Conditions Detected!\r\n");
        }
        else {
            
            // Handling the received UART characters
            if(queue_dequeue(&rx_queue, &rx_char)) {
                // 'f' or 'F' toggles the filter 
                if(rx_char == 'f' || rx_char == 'F') {
                     BMP280_filter_TOGGLE();
                     BMP280_ReadRegs(0xF5, &filter_debug, 1); // JUST FOR DEBUG 
                     if(filter_off) {
                         uart_print("Filter disabled\r\n");
                         filter_off = 0;
                     }
                     else {
                         uart_print("Filter enabled\r\n");
                         filter_off = 1;
                     }
                }
                // 's' or 'S' prints status and history
                else if(rx_char == 's' || rx_char == 'S') {
                    char buffer[128];
                    
                    // Print:  MODE // Filter status //  Reference Pressure
                    sprintf(buffer, "\r\n STATUS REPORT \r\nMode: %s\r\nFilter: %s\r\nP0: %.2f hPa\r\n", 
                            (mode_flag == NORMAL) ? "ACTIVE (NORMAL)" : "ECO (FORCED)", 
                            (filter_off == 1) ? "ON" : "OFF", 
                             P_0);
                    uart_print(buffer);
                    
                    // Print the 10 latest measurements from the queue
                    uart_print("History (Oldest to Newest):\r\n");
                    if (queue_count == 0) {
                        uart_print(" No measurements yet.\r\n");
                    } 
                    else {
                        for (uint32_t i = 0; i < queue_count; i++) {
                            uint32_t idx = (queue_head + i) % HISTORY_SIZE;
                            sprintf(buffer, " [%d]: %.2f hPa\r\n", i + 1, pressure_history[idx].pressure);
                            uart_print(buffer);
                        }
                    }
                    uart_print("----------------------\r\n");
                }
            }
            
            // Modes handling
            if(outer_btn_flag) {
                outer_btn_flag = 0; // drop the flag 
                //uart_print("button r\n");
                if(mode_flag == NORMAL) {
                    uart_print("Entered FORCED MODE\r\n");
                    mode_flag = FORCED;
                    BMP280_change_mode(BMP280_MODE_FORCED);
                    forced_blink_tmr = cnt_50ms;
                    forced_update_tmr = cnt_50ms;
                    gpio_set(P_LED_R, 1); 
                } 
                else if(mode_flag == FORCED) {
                    uart_print("Entered NORMAL MODE\r\n");
                    mode_flag = NORMAL;
                    BMP280_change_mode(BMP280_MODE_NORMAL);
                    BMP280_sb_t_set(BMP280_STANDBY_0_5_MS);
                    normal_blink_tmr = cnt_50ms;
                    normal_update_tmr = cnt_50ms;
                    gpio_set(P_LED_R, 1);
                }
            }

            // Altidute calc logic 
            if(board_btn_flag) {
                board_btn_flag = 0; // Drop the flag raised from the ISR
                
                // Safety check: Calibrate only if we have a valid measurement
                if(pressure > 500.0f) { 
                    P_0 = pressure; // New Pressure reference
                    
                    char cal_buf[100];
                    sprintf(cal_buf, "\r\n Altitude Calibrated! New P0: %.2f hPa \r\n", P_0);
                    uart_print(cal_buf);
                }
            }
            
            // NORMAL MODE 
            if(mode_flag == NORMAL) {
                
                if((cnt_50ms - normal_blink_tmr) >= 5) { // Blink every 250ms (5 * 50ms)
                    normal_blink_tmr = cnt_50ms;  
                    gpio_toggle(P_LED_R); 
                }
                
                if((cnt_50ms - normal_update_tmr) >= 20) { // Update every 1 second (20 * 50ms)
                    normal_update_tmr = cnt_50ms; 
                    
                    BMP280_Read_Temp_Press(&temperature, &pressure); 
                    update_pressure_history(pressure, cnt_50ms); // Store pressure and timestamp
                    calc_h(pressure); // Update altitude 
                    
                    check_alarm_conditions(); // Evaluate all alarm triggers
                    
                    // Print the updated data on the screen
                    char uart_buffer[100];
                    sprintf(uart_buffer, "Temp: %.2f C | Press: %.2f hPa | Alt: %.2f m\r\n", temperature, pressure, altitude);
                    uart_print(uart_buffer);
                }
            }
            
            // FORCED MODE 
            else if(mode_flag == FORCED) {
                
                // Toggle LED every 2 seconds (40 * 50ms)         
                if((cnt_50ms - forced_blink_tmr) >= 40) {
                    forced_blink_tmr = cnt_50ms; 
                    gpio_toggle(P_LED_R);
                }
                
                // Awake sensor before the 5 second mark to complete measurement
                // Max measurement duration is ~43.2 ms for ultra-high res.
                if((cnt_50ms - forced_update_tmr) == 99) {
                    BMP280_change_mode(BMP280_MODE_FORCED);
                }
                        
                // Read measurement, update queue and print
                if((cnt_50ms - forced_update_tmr) >= 100) { // Update every 5 seconds
                    forced_update_tmr = cnt_50ms; 
                    
                    BMP280_Read_Temp_Press(&temperature, &pressure);
                    update_pressure_history(pressure, cnt_50ms); // Store pressure and timestamp
                    calc_h(pressure); 
                    
                    check_alarm_conditions(); // Evaluate all alarm triggers
                    
                    char uart_buffer[100];
                    sprintf(uart_buffer, "Temp: %.2f C | Press: %.2f hPa | Alt: %.2f m\r\n", temperature, pressure, altitude);
                    uart_print(uart_buffer);
                }
            }
        }
    } 
}