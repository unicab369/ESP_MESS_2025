#include "button_click.h"
#include "esp_timer.h"
#include "driver/gpio.h"

#define DEBOUNCE_TIME 50000      
#define DOUBLE_CLICK_TIME 500000
#define LONG_PRESS_TIME 2000000
#define LONG_PRESS_REPEAT_TIME 200000

static gpio_num_t button_gpio;
static button_event_callback_t event_callback;
static uint64_t last_press_time = 0;
static uint64_t last_event_time = 0;
static bool button_pressed = false;
static bool waiting_for_double_click = false;
static bool long_press_detected = false;

void button_click_setup(gpio_num_t gpio, button_event_callback_t callback) {
    button_gpio = gpio;
    event_callback = callback;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << button_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void button_click_run(void) {
    uint64_t current_time = esp_timer_get_time();
    bool current_state = gpio_get_level(button_gpio) == 0;

    if (current_time - last_press_time < DEBOUNCE_TIME) return;

    if (current_state != button_pressed) {
        button_pressed = current_state;
        
        if (button_pressed) {
            last_press_time = current_time;
            long_press_detected = false;
        } else {
            if (waiting_for_double_click) {
                event_callback(BUTTON_EVENT_DOUBLE_CLICK, 0);
                waiting_for_double_click = false;
            } else if (!long_press_detected) {
                waiting_for_double_click = true;
                last_event_time = current_time;
            }
        }
    }
    else if (waiting_for_double_click && (current_time - last_event_time > DOUBLE_CLICK_TIME)) {
        event_callback(BUTTON_EVENT_SINGLE_CLICK, 0);
        waiting_for_double_click = false;
    }
    else {
        uint64_t time_diff = current_time - last_press_time;

        if (button_pressed && (time_diff > LONG_PRESS_TIME)) {
            if (!long_press_detected) {
                event_callback(BUTTON_EVENT_LONG_PRESS, 0);
                long_press_detected = true;
                last_event_time = current_time;
            } else if (current_time - last_event_time > LONG_PRESS_REPEAT_TIME) {
                event_callback(BUTTON_EVENT_LONG_PRESS, time_diff - LONG_PRESS_TIME);
                last_event_time = current_time;
            }
        }
    }
}
