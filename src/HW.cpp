#include "HW.h"

volatile int timems = 0;
volatile bool state = false;

volatile SemaphoreHandle_t button_sem1 = xSemaphoreCreateBinary();
volatile SemaphoreHandle_t button_sem2 = xSemaphoreCreateBinary();

void HW::handler(uint gpio, uint32_t eventmask) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    uint32_t newtime = time_us_32() / 1000;

    uint ROT_SW = 12;
    uint ROT_A = 10;

    if (gpio == ROT_SW && eventmask == GPIO_IRQ_EDGE_FALL) {
        if ((newtime - timems) > DEBOUNCE_TIME) {
            state = true;
            xSemaphoreGiveFromISR(button_sem1, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }

    if (gpio == ROT_SW && eventmask == GPIO_IRQ_EDGE_RISE) {
        state = false;
    }

    if (gpio == ROT_A) {
        if ((newtime - timems) > DEBOUNCE_TIME_ROT) {
            xSemaphoreGiveFromISR(button_sem2, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }

    timems = newtime;
}

void HW::button_task(void *params) {
    auto par = (task_params *) params;

    uint ROT_B = 11;

    const uint led_pin = 21;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    while (true) {
        if (xSemaphoreTake(button_sem1, pdMS_TO_TICKS(5)) == pdTRUE) {
            xSemaphoreGive(par->sw);
        }

        if (xSemaphoreTake(button_sem2, pdMS_TO_TICKS(5)) == pdTRUE) {
            if (gpio_get(ROT_B)) {
                //xSemaphoreGive("counter_clockwise");
                xSemaphoreGive(par->minus);
            }
            if (!gpio_get(ROT_B)) {
                //xSemaphoreGive("clockwise");
                xSemaphoreGive(par->plus);
            }
        }
        vTaskDelay(BUTTONS_TASK_DELAY);
    }
}

void HW::init_gpio(bool up, bool out, int nr, ...) {
    va_list args;
    va_start(args, nr);
    for(int i = 0; i < nr; i++) {
        uint arg = va_arg(args, uint);
        gpio_init(arg);
        // pull up or pull down
        up ? gpio_pull_up(arg) : gpio_pull_down(arg);
        // output or input
        out ? gpio_set_dir(arg, GPIO_OUT) : gpio_set_dir(arg, GPIO_IN);
        // interrupt for button
        if (arg == 9 || arg == 12) {
            gpio_set_irq_enabled_with_callback(arg, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &handler);
        }
        if (arg == 10) {
            gpio_set_irq_enabled_with_callback(arg, GPIO_IRQ_EDGE_RISE, true, &handler);
        }
    }
    va_end(args);
}

HW::HW() {
    binary_semaphore_switch = xSemaphoreCreateBinary();
    binary_semaphore_plus = xSemaphoreCreateBinary();
    binary_semaphore_minus = xSemaphoreCreateBinary();
}