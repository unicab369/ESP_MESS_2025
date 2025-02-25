#include "led_toggle.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static gpio_num_t led_gpio;
static int toggle_count = 0;
static bool led_state = false;
static bool is_toggling = true;

static uint64_t last_toggle_time = 0;
static uint64_t last_wait_time = 0;
static uint32_t conf_toggle_usec = 200000;  // micro seconds
static uint32_t conf_wait_usec = 2000000;   // micro seconds
static int conf_toggle_max = 6;

void led_toggle_init(gpio_num_t gpio, led_config_t config) {
    led_gpio = gpio;

    // Configure the LED GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << led_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    led_state = !config.invert_state;
    conf_toggle_max = config.blink_count*2;
    conf_toggle_usec = config.toggle_time_ms*1000;
    conf_wait_usec = config.wait_time_ms*1000;

    last_toggle_time = esp_timer_get_time();
    last_wait_time = esp_timer_get_time();
}

void led_toggle_update(void) {
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