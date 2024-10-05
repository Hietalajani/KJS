
#include "I2C.h"
//////////////////////////////////////////////////////////////

uint8_t I2C::USERNAME[13];
uint8_t I2C::PASSWORD[9];
uint8_t I2C::IIPEE[13];


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

uint8_t I2C::write_eeprom(uint8_t *un, uint8_t *pw, uint8_t *ip) {
    uint8_t address[3] = {0, 0, 0x50};
    i2c_write_blocking(i2c0, EEPROM_ADDR, address, 3, true);
//    sleep_ms(5);
//    int ret = i2c_write_blocking(i2c0, EEPROM_ADDR, un, 13, true);
//    sleep_ms(5);
//    i2c_write_blocking(i2c0, EEPROM_ADDR, pw, 9, true);
//    sleep_ms(5);
//    i2c_write_blocking(i2c0, EEPROM_ADDR, ip, 13, false);
//    sleep_ms(5);
//    printf("RET: %d", ret);
//    return ret;
}

void I2C::read_eeprom() {
    uint8_t helo;
    uint8_t address[2] = {0, 0};
    i2c_write_blocking(i2c0, EEPROM_ADDR, address, 2, true);
    sleep_ms(5);
    i2c_read_blocking(i2c0, EEPROM_ADDR, &helo, 1, false);
    printf("EEPROM: %x\n", helo);
//    i2c_read_blocking(i2c0, EEPROM_ADDR, USERNAME, 13, true);
//    i2c_read_blocking(i2c0, EEPROM_ADDR, PASSWORD, 9, true);
//    i2c_read_blocking(i2c0, EEPROM_ADDR, IIPEE, 13, false);
}


void I2C::update_oled(ssd1306 display, double temp, uint16_t co2, double rh, uint16_t fanspeed, uint16_t pressure, int set_pressure, bool auto_m) {
    extern int16_t cursor_position;
    display.fill(0);
    const uint8_t cursor4x8[] =
            {
                    0xff, 0x7e, 0x3c, 0x18
            };

    mono_vlsb cr(cursor4x8, 4, 8);
    extern int menu_state;
    if (auto_m) menu_state = 6;

    switch (menu_state) {
        case 0:
            if (cursor_position == 60) display.blit(cr, 80, 10);
            else display.blit(cr, 0, cursor_position);
            display.text("<MAIN MENU>", 5, 0);
            display.text("Temp", 5, 10);
            display.text("CO2", 5, 20);
            display.text("RH", 5, 30);
            display.text("Fanspeed", 5, 40);
            display.text("Pressure", 5, 50);
            display.text("Auto", 85, 10);
            display.show();
            break;
        case 1:
            display.fill(0);
            display.text("<TEMPERATURE>", 5, 0);
            display.text(std::to_string(temp) + "C", 5, 25);
            display.show();
            break;
        case 2:
            display.fill(0);
            display.text("<CO2>", 5, 0);
            display.text(std::to_string(co2) + "ppm", 5, 25);
            display.show();
            break;
        case 3:
            display.fill(0);
            display.text("<REL. HUMIDITY>", 5, 0);
            display.text(std::to_string(rh) + "%", 5, 25);
            display.show();
            break;
        case 4:
            display.fill(0);
            display.text("<FANSPEED>", 5, 0);
            display.rect(5, 25, 100, 20, 1);
            display.text("val:" + std::to_string(fanspeed), 30, 50, 1);
            for (int i = 0; i <= fanspeed; i++) {
                display.line(6+i, 26, 6+i, 44, 1);
            }
            display.show();
            break;
        case 5:
            display.fill(0);
            display.text("<PRESSURE>", 5, 0);
            display.text(std::to_string(pressure) + "Pa", 5, 25);
            display.show();
            break;
        case 6:
            display.fill(0);
            display.text("<AUTO MODE>", 5, 0);
            display.text("Value: " + std::string(auto_m ? "ON" : "OFF"), 5, 15);
            display.text("Pressure: " + std::to_string(set_pressure) + "Pa", 5, 25);
            display.show();
            break;
        default:
            break;

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

