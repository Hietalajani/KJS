#include "HW.h"

volatile int timems = 0;
volatile int16_t cursor_position = 10;
volatile bool state = false;
volatile int menu_state = 0;
extern bool auto_m;
extern bool fan_write;
extern int fan_speed;
extern int set_pressure;

//pwm_config HW::pwm_get_config_struct() {
//    const uint32_t f_pwm = PWM_FREQUENCY;
//
//    uint32_t sys_clock = clock_get_hz(clk_sys);
//    uint32_t divider = sys_clock / MHZ_1;
//    uint32_t top = MHZ_1 / f_pwm - 1;
//
//    pwm_config config = {
//            sys_clock,
//            divider,
//            top
//    };
//
//    return config;
//}

void HW::handler(uint gpio, uint32_t eventmask) {
    uint32_t newtime = time_us_32() / 1000;

    if (gpio == 9) {
        if(eventmask == GPIO_IRQ_EDGE_FALL) {
            if ((newtime - timems) > DEBOUNCE_TIME && !state) {
                state = true;
                auto_m = !auto_m;
            }
        }
        else {
            state = false;
        }
    }

    if (gpio == 10) {
        if ((newtime - timems) > DEBOUNCE_TIME) {
            if (gpio_get(11)) {
                if (menu_state == 0){
                    if (cursor_position + 10 > 60) cursor_position = 10;
                    else cursor_position += 10;
                }
                else if (menu_state == 4) {
                    if (fan_speed + 50 > 1000) fan_speed = 1000;
                    else fan_speed += 50;
                    fan_write = true;
                    auto_m = false;
                }
                else if (menu_state == 6) {
                    if (set_pressure + 1 > 125) set_pressure = 125;
                    else set_pressure += 1;
                }
            } else {
                if (menu_state == 0){
                    if (cursor_position - 10 < 10) cursor_position = 60;
                    else cursor_position -= 10;
                }
                else if (menu_state == 4) {
                    if (fan_speed - 50 < 0) fan_speed = 0;
                    else fan_speed -= 50;
                    fan_write = true;
                    auto_m = false;
                }
                else if (menu_state == 6) {
                    if (set_pressure - 1 < 0) set_pressure = 0;
                    else set_pressure -= 1;
                }
            }
        }
    }

    else if (gpio == 12) {
        if(eventmask == GPIO_IRQ_EDGE_FALL) {
            if ((newtime - timems) > DEBOUNCE_TIME && !state) {
                state = true;
                if (menu_state == 0) { menu_state = cursor_position / 10; }
                else menu_state = 0;
            }
        }
        else {
            state = false;
        }
    }
    timems = newtime;
}

//HW::HW(bool up, bool out, int nr, ...) {
//    va_list args;
//    va_start(args, nr);
//    for(int i = 0; i < nr; i++) {
//        uint arg = va_arg(args, uint);
//        gpio_init(arg);
//        // pull up or pull down
//        up ? gpio_pull_up(arg) : gpio_pull_down(arg);
//        // output or input
//        out ? gpio_set_dir(arg, GPIO_OUT) : gpio_set_dir(arg, GPIO_IN);
//        // interrupt for button
//        if (arg == 9 || arg == 12) {
//            gpio_set_irq_enabled_with_callback(arg, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &handler);
//        }
//        if (arg == 10) gpio_set_irq_enabled_with_callback(arg, GPIO_IRQ_EDGE_RISE, true, &handler);
//            // leds get a pwm function for less future-sight
//        else if (arg == 20 || arg == 21 || arg == 22) {
//            gpio_set_function(arg, GPIO_FUNC_PWM);
//            uint slice_num = pwm_gpio_to_slice_num(arg);
//            pwm_config config = pwm_get_config_struct();
//            pwm_init(slice_num, &config, true);
//        }
//    }
//    va_end(args);
//}