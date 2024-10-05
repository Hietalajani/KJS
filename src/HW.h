
#ifndef WEEDHILLS_H
#define WEEDHILLS_H

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include "pico/stdlib.h"
// #include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM_FREQUENCY 1000
#define MHZ_1 1000000UL
#define DEBOUNCE_TIME 120

#define ROTARY_ENCODER_PIN_A 1
#define ROTARY_ENCODER_PIN_B 2


class HW {
public:

//    HW(bool up, bool out, int nr, ...);
//    static pwm_config pwm_get_config_struct();

private:
    static void handler(uint gpio, uint32_t eventmask);
};
#endif //WEEDHILLS_H




