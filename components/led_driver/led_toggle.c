#include "led_toggle.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "GPIO_TOGGLE";

static gpio_num_t led_gpio;
static int toggle_count = 0;
static bool led_state = false;
static bool is_toggling = false;

static uint64_t last_toggle_time = 0;
static uint64_t last_wait_time = 0;
static uint32_t conf_toggle_usec = 200000;  // micro seconds
static uint32_t conf_wait_usec = 0;   // micro seconds
static int conf_toggle_max = 6;

static gpio_toggle_t led_config = {
    .blink_count = 1,
    .toggle_time_ms = 100,
    .wait_time_ms = 1000,
    .invert_state = true,
};

void led_toggle_setup(gpio_num_t gpio) {
    led_gpio = gpio;

    // Configure the LED GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << led_gpio),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

void led_toggle_restart() {
    // reset values
    last_toggle_time = esp_timer_get_time();
    last_wait_time = esp_timer_get_time();
    is_toggling = true;

    led_state = !led_config.invert_state;
    conf_toggle_max = led_config.blink_count*2;
    conf_toggle_usec = led_config.toggle_time_ms*1000;
    conf_wait_usec = led_config.wait_time_ms*1000;
}

void led_toggle_pulses(uint8_t count, uint32_t repeat_duration) {
    led_config.blink_count = count;
    led_config.wait_time_ms = repeat_duration;
    led_toggle_restart();
}

void led_toggle_off() {
    conf_wait_usec = 0;
    is_toggling = false;
}

void led_toggle_value(bool onOff) {
    led_toggle_off();
    gpio_set_level(led_gpio, onOff);
}

void led_toggle_switch() {
    led_toggle_off();
    bool readState = gpio_get_level(led_gpio);
    gpio_set_level(led_gpio, !readState);
}

void led_toggle_loop(void) {
    //! conf_wait_usec == 0 means no repeat
    if (conf_wait_usec == 0 && !is_toggling) return;
    uint64_t current_time = esp_timer_get_time();

    if (is_toggling) {
        // Toggle the LED every conf_toggle_usec
        if (current_time - last_toggle_time >= conf_toggle_usec) {
            led_state = !led_state;
            gpio_set_level(led_gpio, led_state);
            last_toggle_time = current_time;
            toggle_count++;

            // If we've toggled enough times, switch to waiting
            if (toggle_count >= conf_toggle_max) {
                is_toggling = false;
                toggle_count = 0;
                last_wait_time = current_time;
            }
        }
    } else {
        // Wait for conf_wait_usec before starting toggling again
        if (current_time - last_wait_time >= conf_wait_usec) {
            is_toggling = true;
            last_toggle_time = current_time;
        }
    }
}