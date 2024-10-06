
#include "I2C.h"
//////////////////////////////////////////////////////////////

uint8_t I2C::USERNAME[13];
uint8_t I2C::PASSWORD[9];
uint8_t I2C::IIPEE[13];
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

void I2C::write_eeprom(sensor_data data) {
    uint8_t address[3] = {0, 0, 0x50};
    uint8_t hi = (data.co2 >> 8) & 0xFF;
    uint8_t lo = (data.co2 >> 0) & 0xFF;
    uint8_t values[2] = {hi, lo};

    i2c_write_blocking(i2c0, EEPROM_ADDR, address, 3, true);
    sleep_ms(5);
    int ret = i2c_write_blocking(i2c0, EEPROM_ADDR, values, 2, true);
    printf("RET: %d\n", ret);
}

uint16_t I2C::read_eeprom() {
    uint8_t helo[2];
    uint16_t co2;
    uint8_t address[2] = {0, 0};
    i2c_write_blocking(i2c0, EEPROM_ADDR, address, 2, true);
    sleep_ms(5);
    i2c_read_blocking(i2c0, EEPROM_ADDR, &helo[0], 1, true);
    i2c_read_blocking(i2c0, EEPROM_ADDR, &helo[1], 1, false);

    co2 = (helo[0] << 8) | helo[1];

    return co2;
}

void I2C::eeprom_task(void *params) {
    auto par = (oled_params *) params;
    sensor_data data{};
    if (xQueueReceive(par->q, static_cast <void *> (&data), portMAX_DELAY) == pdTRUE) {
        I2C::write_eeprom(data);
    }
}

void I2C::update_oled(void *params) {
    auto par = (task_params *) params;
    ssd1306 display = par->display;
    sensor_data data = {
            .temp = 0,
            .co2 = 0,
            .rh = 0,
            .fanspeed = 0,
            .pressure = 0
    };

    extern int16_t cursor_position;

    display.fill(0);
    const uint8_t cursor4x8[] =
            {
                    0xff, 0x7e, 0x3c, 0x18
            };

    mono_vlsb cr(cursor4x8, 4, 8);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(50));
        display.fill(0);

        xQueueReceive(par->SensorToOLED_que, static_cast <void *> (&data), pdMS_TO_TICKS(5));

        if (menu_state == 0) {
            if (xSemaphoreTake(par->sw, 5) == pdTRUE) {
                menu_state = 4;
            }
        }

        switch (menu_state) {
            case 0:
                if (cursor_position == 60) display.blit(cr, 80, 10);
                else display.blit(cr, 0, cursor_position);
                display.text("<READINGS>", 5, 0);
                display.text("Temp: " + std::to_string(data.temp), 5, 10);
                display.text("CO2: " + std::to_string(data.co2), 5, 20);
                display.text("RH: " + std::to_string(data.rh), 5, 30);
                display.text("Fanspeed: " + std::to_string(data.fanspeed), 5, 40);
                display.text("Pressure: " + std::to_string(data.pressure), 5, 50);
                display.show();
                break;
            case 4:
                display.fill(0);
                display.text("<SET CO2>", 5, 0);
                display.rect(5, 25, 100, 20, 1);
                display.text("val:" + std::to_string(data.co2), 30, 50, 1);
                for (int i = 0; i <= data.co2; i++) {
                    display.line(6 + i, 26, 6 + i, 44, 1);
                }
                display.show();
                break;
//            case 5:
//                display.fill(0);
//                display.text("<PRESSURE>", 5, 0);
//                display.text(std::to_string(data.pressure) + "Pa", 5, 25);
//                display.show();
//                break;

            default:
                break;

        }
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

