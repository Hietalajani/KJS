//
// Created by Drago on 05/10/2024.
//

#ifndef RP2040_FREERTOS_IRQ_SENSOR_TASK_H
#define RP2040_FREERTOS_IRQ_SENSOR_TASK_H
#define TIME_TO_PROC 1000 // example definition for 1 second
#define MAX_FANSPEED 1000
#define MAX_CO2 1500
#define CRITICAL_CO2 2000
#define MIN_CO2 0

#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define SCALE 5
#endif

#define STOP_BITS 1 // for simulator
//#define STOP_BITS 2 // for real system
#include "I2C.h"
#include "ModbusRegister.h"
#include "ModbusClient.h"
#include "PicoOsUart.h"
void sensor_task(void *param);

struct params {
    SemaphoreHandle_t minus;
    SemaphoreHandle_t plus;
    SemaphoreHandle_t set_co2;
    QueueHandle_t SensorToOLED_que; // sensor to oled
    ssd1306 display;
};


#endif //RP2040_FREERTOS_IRQ_SENSOR_TASK_H
