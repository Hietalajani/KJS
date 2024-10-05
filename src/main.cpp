#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306.h"
#include "I2C.h"


#include "hardware/timer.h"
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

#include "blinker.h"

SemaphoreHandle_t gpio_sem;

void gpio_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // signal task that a button was pressed
    xSemaphoreGiveFromISR(gpio_sem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

struct led_params{
    uint pin;
    uint delay;
};

void blink_task(void *param)
{
    auto lpr = (led_params *) param;
    const uint led_pin = lpr->pin;
    const uint delay = pdMS_TO_TICKS(lpr->delay);
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    while (true) {
        gpio_put(led_pin, true);
        vTaskDelay(delay);
        gpio_put(led_pin, false);
        vTaskDelay(delay);
    }
}

void gpio_task(void *param) {
    (void) param;
    const uint button_pin = 9;
    const uint led_pin = 22;
    const uint delay = pdMS_TO_TICKS(250);
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_set_pulls(button_pin, true, false);
    gpio_set_irq_enabled_with_callback(button_pin, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    while(true) {
        if(xSemaphoreTake(gpio_sem, portMAX_DELAY) == pdTRUE) {
            //std::cout << "button event\n";
            gpio_put(led_pin, 1);
            vTaskDelay(delay);
            gpio_put(led_pin, 0);
            vTaskDelay(delay);
        }
    }
}

void serial_task(void *param)
{
    PicoOsUart u(0, 0, 1, 115200);
    Blinker blinky(20);
    uint8_t buffer[64];
    std::string line;
    while (true) {
        if(int count = u.read(buffer, 63, 30); count > 0) {
            u.write(buffer, count);
            buffer[count] = '\0';
            line += reinterpret_cast<const char *>(buffer);
            if(line.find_first_of("\n\r") != std::string::npos){
                u.send("\n");
                std::istringstream input(line);
                std::string cmd;
                input >> cmd;
                if(cmd == "delay") {
                    uint32_t i = 0;
                    input >> i;
                    blinky.on(i);
                }
                else if (cmd == "off") {
                    blinky.off();
                }
                line.clear();
            }
        }
    }
}

void modbus_task(void *param);
void display_task(void *param);
void i2c_task(void *param);
extern "C" {
    void tls_test(void);
}
void tls_task(void *param)
{
    tls_test();
    while(true) {
        vTaskDelay(100);
    }
}
#if 0
int main()
{
    static led_params lp1 = { .pin = 20, .delay = 300 };
    stdio_init_all();
    printf("\nBoot\n");

    gpio_sem = xSemaphoreCreateBinary();
    //xTaskCreate(blink_task, "LED_1", 256, (void *) &lp1, tskIDLE_PRIORITY + 1, nullptr);
    //xTaskCreate(gpio_task, "BUTTON", 256, (void *) nullptr, tskIDLE_PRIORITY + 1, nullptr);
    //xTaskCreate(serial_task, "UART1", 256, (void *) nullptr,
    //            tskIDLE_PRIORITY + 1, nullptr);
#if 0
    xTaskCreate(modbus_task, "Modbus", 512, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);


    xTaskCreate(display_task, "SSD1306", 512, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);
#endif
#if 1
    xTaskCreate(i2c_task, "i2c test", 512, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);
#endif
#if 0
    xTaskCreate(tls_task, "tls test", 6000, (void *) nullptr,
                tskIDLE_PRIORITY + 1, nullptr);
#endif
    vTaskStartScheduler();

    while(true){};
}
#endif

#include <cstdio>
#include "ModbusClient.h"
#include "ModbusRegister.h"

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define STOP_BITS 2 // for real system (pico simualtor also requires 2 stop bits)

#define USE_MODBUS

void modbus_task(void *param) {

    const uint led_pin = 22;
    const uint button = 9;

    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);

    // Initialize chosen serial port
    //stdio_init_all();

    //printf("\nBoot\n");

#ifdef USE_MODBUS
    auto uart{std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, BAUD_RATE, STOP_BITS)};
    auto rtu_client{std::make_shared<ModbusClient>(uart)};
    ModbusRegister rh(rtu_client, 241, 256);
    ModbusRegister t(rtu_client, 241, 257);
    ModbusRegister produal(rtu_client, 1, 0);
    produal.write(100);
    vTaskDelay((100));
    produal.write(100);
#endif

    while (true) {
#ifdef USE_MODBUS
        gpio_put(led_pin, !gpio_get(led_pin)); // toggle  led
        printf("RH=%5.1f%%\n", rh.read() / 10.0);
        vTaskDelay(5);
        printf("T =%5.1f%%\n", t.read() / 10.0);
        vTaskDelay(3000);
#endif
    }


}

#include "ssd1306os.h"
void display_task(void *param)
{
    auto i2cbus{std::make_shared<PicoI2C>(1, 400000)};
    ssd1306os display(i2cbus);
    display.fill(0);
    display.text("Boot", 0, 0);
    display.show();
    while(true) {
        vTaskDelay(100);
    }

}

void i2c_task(void *param) {
    auto i2cbus{std::make_shared<PicoI2C>(0, 100000)};

    const uint led_pin = 21;
    const uint delay = pdMS_TO_TICKS(250);
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    uint8_t buffer[64] = {0};
    i2cbus->write(0x50, buffer, 2);

    auto rv = i2cbus->read(0x50, buffer, 64);
    printf("rv=%u\n", rv);
    for(int i = 0; i < 64; ++i) {
        printf("%c", isprint(buffer[i]) ? buffer[i] : '_');
    }
    printf("\n");

    buffer[0]=0;
    buffer[1]=64;
    rv = i2cbus->transaction(0x50, buffer, 2, buffer, 64);
    printf("rv=%u\n", rv);
    for(int i = 0; i < 64; ++i) {
        printf("%c", isprint(buffer[i]) ? buffer[i] : '_');
    }
    printf("\n");

    while(true) {
        gpio_put(led_pin, 1);
        vTaskDelay(delay);
        gpio_put(led_pin, 0);
        vTaskDelay(delay);
    }


}


// ---------------------------------- WEEDHILL PROJECT MAIN -------------------------------------------------


//
// Created by hiets on 28.2.2024.
//

//#include "main.h"
//volatile int fan_speed = 0;
//volatile bool fan_write = false;
//volatile bool auto_m = false;
//volatile int set_pressure = 35;
//static const char *topic = "controller/status";
//static const char *topicsub = "controller/settings";
//
//bool in_range(int low, int high, int pressure){
//    return (low <= pressure && pressure <= high);
//}
//
//void messageArrived(MQTT::MessageData &md) {
//    int nro = 0;
//    MQTT::Message &message = md.message;
//    char *buf = (char *) message.payload;
//    buf[message.payloadlen] = '\0';
//
//    // sulje silmäsi keijo
//    char * ptr = strstr(buf, ":");
//    ptr = strstr(++ptr, ":");
//    ptr += 2;
//    sscanf(ptr, "%d", &nro);
//    if (strstr(buf, "true") == nullptr) auto_m = false;
//    else auto_m = true;
//    if (auto_m) set_pressure = nro;
//    else { fan_speed = nro * 10; fan_write = true; }
//}
//
//int main() {
//    stdio_init_all();
//
//    auto uart{std::make_shared<PicoUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, 9600, STOP_BITS)};
//    auto rtu_client{std::make_shared<ModbusClient>(uart)};
//    ModbusRegister rh(rtu_client, 241, 256);
//    ModbusRegister temp(rtu_client, 241, 257);
//    ModbusRegister co2(rtu_client, 240, 256);
//    auto modbus_poll = make_timeout_time_ms(10000);
//    ModbusRegister produal(rtu_client, 1, 0);
//    ModbusRegister fancounter(rtu_client, 1, 0, false);
//    produal.write(0);
//    sleep_ms((100));
//    produal.write(0);
//
//    I2C fan;
//    fan.init_i2c();
//    I2C::Sensirion s;
//
//    // MQTT SHIZZLES
//    uint8_t username[13] = "SmartIoTMQTT";
//    username[12] = '\0';
//    uint8_t pw[9] = "SmartIoT";
//    pw[8] = '\0';
//    uint8_t ip[13] = "192.168.1.10";
//    ip[12] = '\0';
//
//    I2C::write_eeprom(username, pw, ip);
//    I2C::read_eeprom();
//
//    printf("USERNAME: %s PASSWORD: %s IP: %s\n", I2C::USERNAME, I2C::PASSWORD, I2C::IIPEE);
////
////    IPStack ipstack("SmartIoTMQTT", "SmartIoT");
////    auto client = MQTT::Client<IPStack, Countdown>(ipstack);
////
////    int rc = ipstack.connect("192.168.1.10", 1883);
////    if (rc != 1) {
////        printf("rc from TCP connect is %d\n", rc);
////    }
////
////    printf("MQTT connecting\n");
////    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
////    data.MQTTVersion = 3;
////    data.clientID.cstring = (char *) "PicoW-sample";
////    rc = client.connect(data);
////    if (rc != 0) {
////        printf("rc from MQTT connect is %d\n", rc);
////        while (true) {
////            tight_loop_contents();
////        }
////    }
////    printf("MQTT connected\n");
////    rc = client.subscribe(topicsub, MQTT::QOS2, messageArrived);
////    if (rc != 0) {
////        printf("rc from MQTT subscribe is %d\n", rc);
////    }
////    printf("MQTT subscribed\n");
//
//    auto mqtt_send = make_timeout_time_ms(2000);
//    char buf[100];
//    HW(true, false, 4, 9, 10, 11, 12);
//    ssd1306 display(i2c1);
//
//    while (true) {
//        double temperature = temp.read() / 10.0;
//        int co = co2.read();
//        double relhum = rh.read() / 10.0;
//        int pressure = s.get_result();
//
//        I2C::update_oled(display, temperature, co, relhum, fan_speed / 10, pressure, set_pressure, auto_m);
//        if (auto_m) {
//            // Auto mode, set pressure. Change fan speed to achieve wanted pressure
//            if (!in_range(pressure - SCALE, pressure + SCALE, set_pressure)) {
//                if (pressure > set_pressure) {
//                    if (fan_speed - 20 < 0) fan_speed = 0;
//                    else fan_speed -= 20;
//                } else if (pressure < set_pressure) {
//                    if (fan_speed + 20 > 1000) fan_speed = 1000;
//                    else fan_speed += 20;
//                }
//                fan_write = true;
//            }
//        }
//
//        if (fan_write) {
//            produal.write(fan_speed);
//            fan_write = false;
//        }
//
//        if (time_reached(modbus_poll)) {
//            s.readSensor(MEASURE_PRESSURE);
//            modbus_poll = delayed_by_ms(modbus_poll, 10000);
//            printf("RH =%5.1f%%\n", relhum);
//            printf("TEMP =%5.1fC\n", temperature);
//            printf("CO2 = %d ppm\n", co);
//        }
//
////        if (time_reached(mqtt_send)) {
////            mqtt_send = delayed_by_ms(mqtt_send, 2000);
////            if (!client.isConnected()) {
////                printf("Not connected...\n");
////                rc = client.connect(data);
////                if (rc != 0) {
////                    printf("rc from MQTT connect is %d\n", rc);
////                }
////
////            }
////
////            MQTT::Message message{};
////            message.retained = false;
////            message.dup = false;
////            message.payload = (void *) buf;
////
////            std::ostringstream oss;
////            oss << "{\"auto\":" << std::boolalpha << auto_m
////                << ", \"pressure\":" << pressure
////                << ", \"speed\":" << fan_speed / 10
////                << ", \"co2\":" << co
////                << ", \"rh\":" << relhum
////                << ", \"temp\":" << temperature
////                << "}";
////
////            std::string jsonString = oss.str();
////            sprintf(buf, "%s", jsonString.c_str());
////            message.qos = MQTT::QOS0;
////            message.payloadlen = strlen(buf);
////            buf[message.payloadlen] = '\0';
////            client.publish(topic, message);
////        }
////
////        cyw43_arch_poll(); // obsolete? - see below
////        client.yield(100); // socket that client uses calls cyw43_arch_poll()
//    }
//}


#if 1
int main() {
    stdio_init_all();
    I2C ob;
    ob.init_i2c();
    ssd1306 display(i2c1);
    QueueHandle_t oled_queue = xQueueCreate(5, sizeof(sensor_data));
    oled_params oled_p = { .display = display, .q = oled_queue};




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

    xTaskCreate(I2C::update_oled, "OLED", 512, (void *) &oled_p, tskIDLE_PRIORITY + 1, nullptr);

    // ------------------------------------- MINIMUM REQUIREMENTS DONE -----------------------------------------
    //
    //  THINGSPEAK THINGS


    vTaskStartScheduler();

    while (true) {};
}

#endif