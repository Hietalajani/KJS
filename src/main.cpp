#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306.h"
#include "I2C.h"
#include "Sensor_task.h"
#include "WIFI.h"

#include "hardware/timer.h"
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

QueueHandle_t api_que = xQueueCreate(5, sizeof(char) * 1000);

#if 1
int main() {
    stdio_init_all();
    I2C::init_i2c();
    ssd1306 display(i2c1);

    HW ob;
    HW::init_gpio(true, false, 1, 12);
    HW::init_gpio(false, false, 2, 10, 11);
    HW::init_gpio(false, true, 1, 27);

    Fmutex mutex;

    QueueHandle_t oled_queue = xQueueCreate(5, sizeof(sensor_data));
    QueueHandle_t eeprom_queue = xQueueCreate(5, sizeof(sensor_data));
    QueueHandle_t range_queue = xQueueCreate(5, sizeof(sensor_data));
    QueueHandle_t wifi_queue = xQueueCreate(5, sizeof(sensor_data));
    static task_params spr {
            .minus = ob.binary_semaphore_minus,
            .plus = ob.binary_semaphore_plus,
            .sw = ob.binary_semaphore_switch,
            .SensorToOLED_que = oled_queue,
            .SensorToEEPROM_que = eeprom_queue,
            .SensorToRANGE_que = range_queue,
            .API_QUE = api_que,
            .SensorTOWIFI_que = wifi_queue,
            .display = display,
            .mutex = mutex
    };

    TimerHandle_t API_calls = xTimerCreate(
            "Update Thing speak data and read que",
            TIMER_TIMEOUT,
            pdTRUE,
            nullptr,
            API_callback
    );
    /*TimerHandle_t Update_TS = xTimerCreate(
            "Update Thing speak data",
            TIMER_TIMEOUT,
            pdTRUE,
            nullptr,
            update_ts_callback
    );*/


    xTaskCreate(I2C::update_oled, "OLED", 512, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(I2C::eeprom_task, "EEPROM", 512, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(sensor_task, "SENSOR", 512, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(HW::button_task, "BUTTONS", 512, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(api_task, "API Calls", 1024, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);
    xTimerStart(API_calls, pdMS_TO_TICKS(15000));

    vTaskStartScheduler();

    while (true) {};
}

#endif