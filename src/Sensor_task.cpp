#include "Sensor_task.h"

void send_data(double temp, uint16_t co, uint16_t co2, double rh, uint16_t fanspeed, uint16_t pressure, QueueHandle_t queue, int co2_change){
    sensor_data s = {
            .temp = temp,
            .co2 = co,
            .set_co2 = co2,
            .co2_change = co2_change,
            .rh = rh,
            .fanspeed = fanspeed,
            .pressure = pressure
    };
    xQueueSend(queue, (void *) &s, pdMS_TO_TICKS(10));
}

bool in_range(int lo, int hi, int co2){
    return (co2 >= lo && co2 <= hi);
}

void sensor_task(void *param){

    auto *spr = (task_params *) param;
    QueueHandle_t sensor_queue = spr->SensorToOLED_que;
    QueueHandle_t eeprom_queue = spr->SensorToEEPROM_que;
    SemaphoreHandle_t minus = spr->minus;
    SemaphoreHandle_t plus = spr->plus;
    SemaphoreHandle_t set_co2_level = spr->sw;

    auto uart{std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, 9600, STOP_BITS)};
    auto rtu_client{std::make_shared<ModbusClient>(uart)};

    //Timeout for modbus
    TimeOut_t xTimeOut;
    auto modbus_poll = make_timeout_time_ms(CO2_PUMPING_CHECK);
//    vTaskSetTimeOutState(&xTimeOut);

    // CO2
    ModbusRegister co2(rtu_client, 240, 256);
    uint16_t eeprom_val = I2C::read_eeprom();
    int set_co2;
    if (eeprom_val > 0 && eeprom_val < MAX_CO2) set_co2 = eeprom_val;
    else set_co2 = 500;

    int received_co_change = 0;
    auto critical_co2 = false;

    // Temperature
    ModbusRegister temp(rtu_client, 241, 257);

    // Humidity
    ModbusRegister rh(rtu_client, 241, 256);

    // Fanspeed
    uint16_t fan_speed = 100;
    auto fan_write = false;
    auto eeprom_write = false;
    ModbusRegister produal(rtu_client, 1, 0);
    produal.write(100);
    vTaskDelay(pdMS_TO_TICKS(100));
    produal.write(100);

    // Fancounter
    ModbusRegister fancounter(rtu_client, 1, 0, false);

    I2C::Sensirion s;

    auto range = true;

    while(true){
        double temperature = temp.read() / 10.0;
        uint16_t co = co2.read();
        double relhum = rh.read() / 10.0;
        //Sensirion.
        uint16_t pressure = s.get_result();

        // Queue to read set CO2 level from user.
        if (I2C::menu_state == 4) {
            //printf("Menustate = %d\n", I2C::menu_state);
            if (xSemaphoreTake(minus, pdMS_TO_TICKS(5)) == pdTRUE) {
                if (set_co2 - received_co_change > MIN_CO2) received_co_change -= 10;
            }
            if (xSemaphoreTake(plus, pdMS_TO_TICKS(5)) == pdTRUE) {
                if (set_co2 + received_co_change < MAX_CO2) received_co_change += 10;
            }
            if (xSemaphoreTake(set_co2_level, pdMS_TO_TICKS(3)) == pdTRUE) {
                set_co2 += received_co_change;
                received_co_change = 0;
                I2C::menu_state = 0;

                eeprom_write = true;
            }
        }

        // Checking if CO2 level is higher than 2000
        if (co > CRITICAL_CO2) {
            critical_co2 = true;
        }

        // Running max fan speed until we achieve received CO2 level, or yes?
        if (critical_co2 && co > set_co2 + 15){
            if (fan_speed + 20 > MAX_FANSPEED) fan_speed = MAX_FANSPEED;
            else fan_speed += 20;
            fan_write = true;
        } else {
            critical_co2 = false;
            if (co <= set_co2 + 15) {
                if (fan_speed - 20 < 0) fan_speed = 0;
                else fan_speed -= 20;
                fan_write = true;
            }
        }

        if (!range && co < set_co2) {
            gpio_put(27, true);
            vTaskDelay(pdMS_TO_TICKS(CO2_PUMPING_TIME));
            gpio_put(27, false);
            range = true;
        }

        // Fanspeed 0 - 1000 = 0-100% = 0-10V
        if (fan_write) {
            produal.write(fan_speed);
            fan_write = false;
        }

        //send all data.
        send_data(temperature, co, set_co2, relhum, fan_speed, pressure, sensor_queue, received_co_change);

        if (eeprom_write) {
            send_data(temperature, co, set_co2, relhum, fan_speed, pressure, eeprom_queue, received_co_change);
            eeprom_write = false;
        }

        // Read and printing measured data.
        if (time_reached(modbus_poll)) {
            spr->mutex.lock();
            s.readSensor(MEASURE_PRESSURE);
            spr->mutex.unlock();
//            vTaskSetTimeOutState(&xTimeOut);
            printf("Set_co2 = %d\n", set_co2);
            printf("Fanspeed = %d\n", fan_speed);
            printf("RH =%5.1f%%\n", relhum);
            printf("TEMP =%5.1fC\n", temperature);
            printf("CO2 = %d ppm\n", co);
            modbus_poll = delayed_by_ms(modbus_poll, CO2_PUMPING_CHECK);
            range = in_range(set_co2 - SCALE, set_co2 + SCALE, co);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}