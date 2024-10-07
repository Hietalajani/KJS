
#include "WIFI.h"
SemaphoreHandle_t timer_api_semaphore = xSemaphoreCreateBinary();

void API_callback( TimerHandle_t xTimer){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(timer_api_semaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void api_task(void *param){
    auto send_receive = false;
    while(true) {
        xSemaphoreTake(timer_api_semaphore, pdMS_TO_TICKS(3));
        if (!send_receive) {
            tls_test(SEND);
            send_receive = true;
        }
        else{
            tls_test(QUE_RECEIVE);
            send_receive = false;
        }
    }
}
