//
// Created by hiets on 28.2.2024.
//

#ifndef WEEDHILL_PROJECT_I2C_H
#define WEEDHILL_PROJECT_I2C_H

#include <stdio.h>
#include <array>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "display/ssd1306.h"
#include "display/font_petme128_8x8.h"
#include "display/framebuf.h"
#include "display/mono_vlsb.h"
#include "display/ssd1306.h"
#include "HW.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

// Pins for eeprom and sensirion
#define SENSIRION_ADDR 0x40
#define EEPROM_ADDR 0x50
#define EEPROM_SCL 17
#define EEPROM_SDA 16
#define SENSIRION_SDA 14
#define SENSIRION_SCL 15

// Other defines

#define BAUD_RATE 115200
#define MEASURE_PRESSURE 0xF1

#define SCALE_FACTOR 240.0 //Pa^-1

struct sensor_data {
    double temp;
    uint16_t co2;
    uint16_t set_co2;
    int co2_change;
    double rh;
    uint16_t fanspeed;
    uint16_t pressure;
};

class I2C {
public:
    I2C() = default;
    static void write_eeprom(uint16_t data);
    static uint16_t read_eeprom();
    static void eeprom_task(void *params);
    static int menu_state;

    static void init_i2c();

    static void update_oled(void *params);

    static uint8_t USERNAME[13];
    static uint8_t PASSWORD[9];
    static uint8_t IIPEE[13];

    class Sensirion{
    public:
        //Sensirion() : result(0), data{0,0}, pressure(0), altitude_pressure(0.0){}
        Sensirion();
        int16_t get_result() const;
        double altitude_pressure;
        void readSensor(uint8_t command);
        double pressure_calculation(int16_t result);
    private:
        uint16_t result;
        uint8_t data[3]{};
        int pressure;

    };
};

#endif //WEEDHILL_PROJECT_I2C_H
