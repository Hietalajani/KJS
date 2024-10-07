//
// Created by hiets on 28.2.2024.
//

#ifndef WEEDHILL_PROJECT_WIFI_H
#define WEEDHILL_PROJECT_WIFI_H

#include "FreeRTOS.h"
#include "timers.h"
#include "IPStack.h"
#include "semphr.h"

#define BUFSIZE 2048
#define SEND 1
#define QUE_RECEIVE 0
#define TIMER_TIMEOUT 25000
void API_callback( TimerHandle_t xTimer);
void api_task(void *param);
extern "C" void tls_test(int send_or_receive);

class WIFI {

};


#endif //WEEDHILL_PROJECT_WIFI_H
