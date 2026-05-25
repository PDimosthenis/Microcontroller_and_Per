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
 * - Pin 2 (Diagonal) -> PA_9 
 * * 3. External LED
 * - Long Leg (Anode +)   -> PC_7
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

// --- Custom Inline Circular Buffer for Pressure History ---
#define HISTORY_SIZE 10
float pressure_history[HISTORY_SIZE];
uint32_t queue_head = 0;
uint32_t queue_tail = 0;
uint32_t queue_count = 0;

void update_pressure_history(float new_pressure) {
    pressure_history[queue_tail] = new_pressure;
    queue_tail = (queue_tail + 1) % HISTORY_SIZE;
    
    if (queue_count < HISTORY_SIZE) {
        queue_count++;
    } else {
        queue_head = (queue_head + 1) % HISTORY_SIZE;
    }
}

// Reference pressure for altitude calculation (Initial Standard Value)
volatile float P_0 = 1013.25f;

// Global timer counter updated every 50ms by timer ISR
volatile uint32_t cnt_50ms = 0; 

// Interrupt flags
volatile bool outer_btn_flag = 0;
volatile bool board_btn_flag = 0;

// Global sensor variables
float temperature = 0.0f;
float pressure = 0.0f;
float altitude = 0.0f;

// --- Interrupt Service Routines (ISRs) ---
void timer_isr() { cnt_50ms++; }
void outer_btn_isr() { outer_btn_flag = 1; }
void board_btn_isr() { board_btn_flag = 1; }
void uart_rx_isr(uint8_t rx) {
    if(rx == 'c' || rx == 's' || rx == 'f' || rx == 'C' || rx == 'S' || rx == 'F') {
        queue_enqueue(&rx_queue, rx);
    }
}

// --- Helper Functions ---
void calc_h(float p_current) {
    altitude = 44330.0f * (1.0f - pow((p_current / P_0), (1.0f / 5.255f)));
}

typedef enum {
    NORMAL = 0,
    FORCED = 1
} state_t;

int main() {
    state_t mode_flag = NORMAL; 
    
    uint32_t normal_blink_tmr = 0;
    uint32_t normal_update_tmr = 0;
    uint32_t forced_blink_tmr = 0;
    uint32_t forced_update_tmr = 0;
    
    // --- GPIO Initialization ---
    gpio_set_mode(PA_9, PullDown);
    gpio_set_trigger(PA_9, Rising);
    gpio_set_callback(PA_9, outer_btn_isr);
    
    gpio_set_mode(PC_7, Output);
    
    gpio_set_mode(PC_13, PullDown);
    gpio_set_trigger(PC_13, Rising);
    gpio_set_callback(PC_13, board_btn_isr);
    
    // --- Peripheral Initialization ---
    timer_init(50000); 
    timer_set_callback(timer_isr);
    timer_enable();
    
    queue_init(&rx_queue, QUEUE_SIZE);
    uart_init(115200);
    uart_set_rx_callback(uart_rx_isr);
    uart_enable();
    
    NVIC_SetPriority(EXTI9_5_IRQn, 0);   // Outer btn (Highest)
    NVIC_SetPriority(EXTI15_10_IRQn, 1); // Board btn
    NVIC_SetPriority(SysTick_IRQn, 2);   // Timer    
    NVIC_SetPriority(USART2_IRQn, 3);    // UART (Lowest)
    
    __enable_irq(); 
    
    // Delay for Sensor Power-On
    uint32_t boot_wait = cnt_50ms;
    while((cnt_50ms - boot_wait) < 4) { 
        __NOP(); 
    }
    
    BMP280_Init();
    
    uart_print("\r\nSystem Initialized at NORMAL_MODE\r\n");
    
    BMP280_sb_t_set(BMP280_STANDBY_1000_MS);
        
    normal_blink_tmr = cnt_50ms;
    normal_update_tmr = cnt_50ms;
    
    while(1) {
        
        // ==== MODE SWITCHING LOGIC (PA_9) ====
        if(outer_btn_flag) {
            outer_btn_flag = 0; 
            uart_print("--- BUTTON PRESSED! ---\r\n");
            if(mode_flag == NORMAL) {
                 mode_flag = FORCED;
                 BMP280_change_mode(BMP280_MODE_FORCED);
                 forced_blink_tmr = cnt_50ms;
                 forced_update_tmr = cnt_50ms;
                 gpio_set(PC_7, 1); 
            } 
            else if(mode_flag == FORCED) {
                 mode_flag = NORMAL;
                 BMP280_change_mode(BMP280_MODE_NORMAL);
                 BMP280_sb_t_set(BMP280_STANDBY_1000_MS);
                 normal_blink_tmr = cnt_50ms;
                 normal_update_tmr = cnt_50ms;
                 gpio_set(PC_7, 1);
            }
        }

        // ==== ALTITUDE CALIBRATION LOGIC (PC_13) ====
        if(board_btn_flag) {
            board_btn_flag = 0; 
            
            // Safety check: Calibrate only if we have a valid measurement
            if(pressure > 500.0f) { 
                P_0 = pressure; 
                
                char cal_buf[100];
                sprintf(cal_buf, "\r\n--- Altitude Calibrated! New P0: %.2f hPa ---\r\n", P_0);
                uart_print(cal_buf);
            }
        }
        
        // ==== NORMAL MODE OPERATIONS ====
        if(mode_flag == NORMAL) {
            
            if((cnt_50ms - normal_blink_tmr) >= 5) {
                normal_blink_tmr = cnt_50ms; 
                gpio_toggle(PC_7);
            }
            
            if((cnt_50ms - normal_update_tmr) >= 20) {
                normal_update_tmr = cnt_50ms; 
                
                BMP280_Read_Temp_Press(&temperature, &pressure);
                update_pressure_history(pressure);
                calc_h(pressure); // Update altitude based on P_0
                
                char uart_buffer[100];
                sprintf(uart_buffer, "Temp: %.2f C | Press: %.2f hPa | Alt: %.2f m\r\n", temperature, pressure, altitude);
                uart_print(uart_buffer);
            }
        }
        
        // ==== FORCED MODE OPERATIONS ====
        else if(mode_flag == FORCED) {
                      
            if((cnt_50ms - forced_blink_tmr) >= 40) {
                forced_blink_tmr = cnt_50ms; 
                gpio_toggle(PC_7);
            }
            
            if((cnt_50ms - forced_update_tmr) >= 100) {
                forced_update_tmr = cnt_50ms; 
                
                BMP280_change_mode(BMP280_MODE_FORCED);
                
                uint32_t meas_wait = cnt_50ms;
                while((cnt_50ms - meas_wait) < 2) {
                    __NOP();
                }
                
                BMP280_Read_Temp_Press(&temperature, &pressure);
                update_pressure_history(pressure);
                calc_h(pressure); // Update altitude based on P_0
                
                char uart_buffer[100];
                sprintf(uart_buffer, "Temp: %.2f C | Press: %.2f hPa | Alt: %.2f m\r\n", temperature, pressure, altitude);
                uart_print(uart_buffer);
            }
        }
        
    } 
}