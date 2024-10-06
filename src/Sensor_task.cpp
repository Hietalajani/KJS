#include "Sensor_task.h"

void send_data(double temp, uint16_t co2, double rh, uint16_t fanspeed, uint16_t pressure, QueueHandle_t queue){
    sensor_data s;
    s.temp = temp;
    s.co2 = co2;
    s.rh = rh;
    s.fanspeed = fanspeed;
    s.pressure = pressure;
    xQueueSend(queue, &s, pdMS_TO_TICKS(10));
}

bool in_range(int low, int high, int co2){
    return (low <= co2 && co2 <= high);
}

void sensor_task(void *param){

    auto *spr = (task_params *) param;
    QueueHandle_t sensor_queue = spr->SensorToOLED_que;
    SemaphoreHandle_t minus = spr->minus;
    SemaphoreHandle_t plus = spr->plus;
    SemaphoreHandle_t set_co2_level = spr->sw;

    auto uart{std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, 9600, STOP_BITS)};
    auto rtu_client{std::make_shared<ModbusClient>(uart)};

    //Timeout for moodbuss
    TimeOut_t xTimeOut;
    TickType_t xTicksToWait = pdMS_TO_TICKS(10000);
    vTaskSetTimeOutState(&xTimeOut);

    // CO2
    ModbusRegister co2(rtu_client, 240, 256);
    int set_co2 = co2.read();
    int received_co_change;
    auto critical_co2 = false;

    // Temperature
    ModbusRegister temp(rtu_client, 241, 257);

    // Humidity
    ModbusRegister rh(rtu_client, 241, 256);

    // Fanspeed
    uint16_t fan_speed = 0;
    auto fan_write = false;
    ModbusRegister produal(rtu_client, 1, 0);
    produal.write(0);
    sleep_ms((100));
    produal.write(0);

    // Fancounter
    ModbusRegister fancounter(rtu_client, 1, 0, false);

    // Sensirion, pitäs niiiku varmaa tehdä init muualla mut ku ei
    I2C::init_i2c();
    I2C::Sensirion s;

    while(true){
        double temperature = temp.read() / 10.0;
        uint16_t co = co2.read();
        double relhum = rh.read() / 10.0;
        //Sensirion.
        uint16_t pressure = s.get_result();

        // Queue to read set CO2 level from user.
        if (I2C::menu_state == 4) {
            if (xSemaphoreTake(minus, pdMS_TO_TICKS(5)) == pdTRUE)received_co_change -= 1;
            if (xSemaphoreTake(plus, pdMS_TO_TICKS(5)) == pdTRUE)received_co_change += 1;
            if (xSemaphoreTake(set_co2_level, pdMS_TO_TICKS(5)) == pdTRUE) {
                set_co2 += received_co_change;
                received_co_change = 0;
                I2C::menu_state = 0;
            }
        }

        // Checking if CO2 level is higher than 2000
        if (co > CRITICAL_CO2) {
            critical_co2 = true;
        }
        // Running max fan speed until we achieve received CO2 level, or yes?
        if (critical_co2 && co >= set_co2){
            produal.write(MAX_FANSPEED);
        } else {
            critical_co2 = false;
        }

        // Adjusting the CO2 level to set CO2.
        if (co != set_co2){
            if (!in_range(co - SCALE, co + SCALE, set_co2)) {
                if (co > set_co2) {
                    if (fan_speed - 20 < MIN_CO2) fan_speed = MIN_CO2;
                    else fan_speed -= 20;
                } else if (co < set_co2) {
                    if (fan_speed + 20 > MAX_CO2) fan_speed = MAX_CO2;
                    else fan_speed += 20;
                }
                fan_write = true;
            }
        }

        // Fanspeed 0 - 1000 = 0-100% = 0-10V
        if (fan_write) {
            produal.write(fan_speed);
            fan_write = false;
        }

        //send all data.
        send_data(temperature, co, relhum, fan_speed, pressure, sensor_queue);

        // Read and printing measured data.
        if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) == pdTRUE) {
            s.readSensor(MEASURE_PRESSURE);
            vTaskSetTimeOutState(&xTimeOut);
            printf("RH =%5.1f%%\n", relhum);
            printf("TEMP =%5.1fC\n", temperature);
            printf("CO2 = %d ppm\n", co);
        }
    }
}