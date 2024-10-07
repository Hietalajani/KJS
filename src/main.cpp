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

#include "hardware/timer.h"
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

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
    static task_params spr {
            .minus = ob.binary_semaphore_minus,
            .plus = ob.binary_semaphore_plus,
            .sw = ob.binary_semaphore_switch,
            .SensorToOLED_que = oled_queue,
            .SensorToEEPROM_que = eeprom_queue,
            .SensorToRANGE_que = range_queue,
            .display = display,
            .mutex = mutex
    };


    // CREATE TASKS
    // BUTTON TASK (HW CLASS)
    //  - init buttons
    //  - create ISR -> gives semaphores when activated
    //  - button task controls global variable cursor_position that navigates the menu
    //  - depending on menu state, button task also controls CO2 level

    // MODBUS TASK (MODBUS CLASS?)
    //  - put keijos code inside modbus task lmao
    //  - when task sees something added into the queue, it spanks it to the fan with produal.write hehe

    // I2C TASK (I2C CLASS) -- EEPROM, SENSIRION AND OLED (SEPARATE TASKS?)
    //  - eeprom reads from queue -> writes to eeprom, then waits for queue again
    //  - on startup eeprom read
    //  - sensirion init and is read every time modbus_poll timer PROCS
    //  - OLED init and then refresh x times per second, checking menu_state

    xTaskCreate(I2C::update_oled, "OLED", 512, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(I2C::eeprom_task, "EEPROM", 512, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(sensor_task, "SENSOR", 512, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(HW::button_task, "BUTTONS", 512, (void *) &spr, tskIDLE_PRIORITY + 1, nullptr);

    // ------------------------------------- MINIMUM REQUIREMENTS DONE -----------------------------------------
    //
    //  THINGSPEAK THINGS


    vTaskStartScheduler();

    while (true) {};
}

#endif