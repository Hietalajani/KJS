
#include "I2C.h"
//////////////////////////////////////////////////////////////

int I2C::menu_state = 0;

void I2C::init_i2c() {
    // Eeprom inits
    i2c_init(i2c0, BAUD_RATE);
    gpio_set_function(EEPROM_SCL, GPIO_FUNC_I2C);
    gpio_set_function(EEPROM_SDA, GPIO_FUNC_I2C);
    gpio_pull_up(EEPROM_SCL);
    gpio_pull_up(EEPROM_SDA);
    // OLED inits
#if 1
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(SENSIRION_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSIRION_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSIRION_SDA);
    gpio_pull_up(SENSIRION_SCL);
#endif
}

void I2C::write_eeprom(uint16_t data) {
    uint8_t address[4] = {0, 0};
    uint8_t hi = (data >> 8) & 0xFF;
    uint8_t lo = (data >> 0) & 0xFF;
    address[2] = hi;
    address[3] = lo;

    i2c_write_blocking(i2c0, EEPROM_ADDR, address, 4, false);
}

uint16_t I2C::read_eeprom() {
    uint8_t helo[2];
    uint16_t co2;
    uint8_t address[2] = {0, 0};
    i2c_write_blocking(i2c0, EEPROM_ADDR, address, 2, true);
    sleep_ms(5);
    i2c_read_blocking(i2c0, EEPROM_ADDR, &helo[0], 1, true);
    i2c_read_blocking(i2c0, EEPROM_ADDR, &helo[1], 1, false);

    co2 = helo[0] << 8 | helo[1];

    return co2;
}

void I2C::eeprom_task(void *params) {
    auto par = (task_params *) params;
    QueueHandle_t q = par->SensorToEEPROM_que;

    sensor_data data {
        .temp = 0,
        .co2 = 0,
        .set_co2 = 0,
        .co2_change = 0,
        .rh = 0,
        .fanspeed = 0,
        .pressure = 0
    };

    while (true) {
        if (xQueueReceive(q, static_cast <void *> (&data), portMAX_DELAY) == pdTRUE) {
            uint16_t set_co2 = data.set_co2;
            I2C::write_eeprom(set_co2);
            xQueueReset(q);
        }
    }
}

void I2C::update_oled(void *params) {
    auto par = (task_params *) params;
    ssd1306 display = par->display;
    sensor_data data = {
            .temp = 0,
            .co2 = 0,
            .set_co2 = 0,
            .co2_change = 0,
            .rh = 0,
            .fanspeed = 0,
            .pressure = 0
    };

    display.fill(0);
    const uint8_t cursor4x8[] =
            {
                    0xff, 0x7e, 0x3c, 0x18
            };

    mono_vlsb cr(cursor4x8, 4, 8);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5));
        display.fill(0);

        xQueueReceive(par->SensorToOLED_que, static_cast <void *> (&data), pdMS_TO_TICKS(5));

        if (menu_state == 0) {
            if (xSemaphoreTake(par->sw, 5) == pdTRUE) {
                menu_state = 4;
            }
        }

        par->mutex.lock();
        switch (menu_state) {
            case 0:
                display.text("<READINGS>", 5, 0);
                display.text("Temp: " + std::to_string((int) data.temp) + "C", 5, 10);
                display.text("CO2: " + std::to_string((int) data.co2) + "ppm", 5, 20);
                display.text("RH: " + std::to_string((int) data.rh) + "%", 5, 30);
                display.text("Fan: " + std::to_string((int) (data.fanspeed / 10)) + "%", 5, 40);
                display.text("hPa: " + std::to_string((int) data.pressure), 5, 50);
                display.show();
                break;
            case 4:
                display.text("<SET CO2>", 5, 0);
                display.rect(5, 25, 100, 20, 1);
                display.text("val:" + std::to_string(data.set_co2 + data.co2_change), 30, 50, 1);
                for (int i = 0; i < (data.set_co2 + data.co2_change) / 15; i++) {
                    display.line(6 + i, 26, 6 + i, 44, 1);
                }
                display.show();
                break;
            case 5:
                display.rect(15, 10, 98, 50, 1);
                // S
                display.rect(20, 15, 26, 8, 1, true);
                display.rect(20, 23, 8, 8, 1, true);
                display.rect(20, 31, 26, 8, 1, true);
                display.rect(38, 39, 8, 8, 1, true);
                display.rect(20, 47, 26, 8, 1, true);
                // E
                display.rect(51, 15, 26, 8, 1, true);
                display.rect(51, 23, 8, 32, 1, true);
                display.rect(59, 31, 18, 8, 1, true);
                display.rect(59, 47, 18, 8, 1, true);
                // T
                display.rect(82, 15, 26, 8, 1, true);
                display.rect(91, 23, 8, 32, 1, true);
                display.show();

                vTaskDelay(pdMS_TO_TICKS(2000));
                menu_state = 0;
                break;
            default:
                break;

        }
        par->mutex.unlock();
    }
}

//////////////////////////////////////////////////////////////

I2C::Sensirion::Sensirion() {
    altitude_pressure = 0;
    result = 0;
    pressure = 0;
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;
}

void I2C::Sensirion::readSensor(uint8_t command){
    int16_t sensor_output = 0;
    uint8_t cmd[1] = { 0xF1 };
    // read access byte 0x81
    // trigger measurement command byte 0xF1
    // soft reset 0xFE
    i2c_write_blocking(i2c1, SENSIRION_ADDR, cmd, 1, true);
    i2c_read_blocking(i2c1, SENSIRION_ADDR, data, 3, false);
    sensor_output = ((data[0] << 8) | data[1]);
    pressure_calculation(sensor_output);
}

int16_t I2C::Sensirion::get_result() const {
    return (int16_t) altitude_pressure;
}

double I2C::Sensirion::pressure_calculation(int16_t sensor_output) {
    pressure = sensor_output/SCALE_FACTOR;
    // Altitude Correction
    altitude_pressure = pressure/0.95;
    return altitude_pressure;
}

//////////////////////////////////////////////////////////////

