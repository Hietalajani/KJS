
#include "WIFI.h"
SemaphoreHandle_t timer_api_semaphore = xSemaphoreCreateBinary();

void API_callback( TimerHandle_t xTimer){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(timer_api_semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void api_task(void *param){
    auto *wpr = (task_params *) param;
    QueueHandle_t wifi_queue = wpr->SensorTOWIFI_que;
    auto send_receive = false;
    sensor_data data = {
            .temp = 0,
            .co2 = 0,
            .set_co2 = 0,
            .co2_change = 0,
            .rh = 0,
            .fanspeed = 0,
            .pressure = 0
    };

    while(true) {
        xQueueReceive(wifi_queue, static_cast <void *> (&data), pdMS_TO_TICKS(5));
        std::string combine_string = "&field1=" + std::to_string((int) data.co2) + "&field2=" + std::to_string((int) data.rh) + "&field3=" + std::to_string((int) data.temp) + "&field4=" + std::to_string((int) data.fanspeed) + "&field5=" + std::to_string((int) data.set_co2);
        const char* update_string = combine_string.c_str();
        xSemaphoreTake(timer_api_semaphore, pdMS_TO_TICKS(3));
        if (!send_receive) {
            tls_test(SEND, update_string);
            send_receive = true;
        }
        else{
            tls_test(QUE_RECEIVE, update_string);
            send_receive = false;
        }
    }
}
