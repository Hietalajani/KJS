
#ifndef WEEDHILLS_H
#define WEEDHILLS_H

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <iostream>
#include <sstream>
#include "Sensor_task.h"

#define DEBOUNCE_TIME 80
#define DEBOUNCE_TIME_ROT 5
#define BUTTONS_TASK_DELAY 10

#define ROTARY_ENCODER_PIN_SW 12
#define ROTARY_ENCODER_PIN_A 10
#define ROTARY_ENCODER_PIN_B 11


class HW {
public:
    HW();
    static void init_gpio(bool up, bool out, int nr, ...);
    static void button_task(void *params);
    SemaphoreHandle_t binary_semaphore_switch;
    SemaphoreHandle_t binary_semaphore_plus;
    SemaphoreHandle_t binary_semaphore_minus;
private:
    static void handler(uint gpio, uint32_t eventmask);
};
#endif //WEEDHILLS_H




