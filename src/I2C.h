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
#define SOFT_RESET 0xFE
#define SCALE_FACTOR 240.0 //Pa^-1
#define MAX_BYTES 16
#define FANSPEED_ADDR 0, 0
#define SETPOINT_ADDR 0, 1
#define PRESSURE_ADDR 0, 2
#define AUTO_ADDR 0, 3
#define ERROR_ADDR 0, 4
#define CO2_ADDR 0, 5
#define RH_ADDR 0, 6
#define TEMP_ADDR 0, 7

struct oled_params {
    ssd1306 display;
    QueueHandle_t q;
};

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
    uint8_t get_data(const std::string& type);
    void scroll();
    static int menu_state;

    static void init_i2c();

    static void update_oled(void *params);
    static void set_values(int fanspeed, int temp, int co2, int rh, int pressure);

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

    static struct eeprom_data {
        uint8_t fanspeed_data;
        uint8_t pressure_data;
        uint8_t co2_data;
        uint8_t rh_data;
        uint8_t temp_data;
    } send_data;
private:
    // eeprom


    static uint8_t eeprom_buffer[5];
    // sensirion
};


#endif //WEEDHILL_PROJECT_I2C_H
