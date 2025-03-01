#include "led_toggle.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

#define LEDC_DUTY_RESOLUTION   LEDC_TIMER_10_BIT

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
static bool is_enabled = false;

static gpio_toggle_t led_config = {
    .blink_count = 1,
    .toggle_time_ms = 100,
    .wait_time_ms = 1000,
    .invert_state = true,
};

void led_toggle_setup(gpio_num_t gpio) {
    led_gpio = gpio;

    // //Configure the LED GPIO
    // gpio_config_t io_conf = {
    //     .pin_bit_mask = (1ULL << led_gpio),
    //     .mode = GPIO_MODE_INPUT_OUTPUT,
    //     .pull_up_en = GPIO_PULLUP_DISABLE,
    //     .pull_down_en = GPIO_PULLDOWN_DISABLE,
    //     .intr_type = GPIO_INTR_DISABLE
    // };
    // gpio_config(&io_conf);

    // ledc_timer_config_t timer_conf = {
    //     .speed_mode = LEDC_LOW_SPEED_MODE,
    //     .duty_resolution = LEDC_DUTY_RESOLUTION,
    //     .timer_num = LEDC_TIMER_0,
    //     .freq_hz = 5000,
    //     .clk_cfg = LEDC_AUTO_CLK
    // };
    // ledc_timer_config(&timer_conf);

    // ledc_channel_config_t channel_conf = {
    //     .gpio_num = led_gpio,
    //     .speed_mode = LEDC_LOW_SPEED_MODE,
    //     .channel = LEDC_CHANNEL_0,
    //     .timer_sel = LEDC_TIMER_0,
    //     .duty = 0,
    //     .hpoint = 0
    // };
    // ledc_channel_config(&channel_conf);
}

void led_toggle_restart() {
    //! stop PWM
    // ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);

    // reset values
    last_toggle_time = esp_timer_get_time();
    last_wait_time = esp_timer_get_time();
    is_toggling = true;

    led_state = !led_config.invert_state;
    conf_toggle_max = led_config.blink_count*2;
    conf_toggle_usec = led_config.toggle_time_ms*1000;
    conf_wait_usec = led_config.wait_time_ms*1000;
    is_enabled = true;
}

void led_toggle_pulses(uint8_t count, uint32_t repeat_duration) {
    led_config.blink_count = count;
    led_config.wait_time_ms = repeat_duration;
    led_toggle_restart();
}

void led_toggle_stop() {
    is_enabled = false;
}

void led_toggle_setValue(bool onOff) {
    led_toggle_stop();
    gpio_set_level(led_gpio, onOff);
}

static void toggle() {
    led_state = !led_state;

    // gpio_set_level(led_gpio, led_state);

    uint32_t duty = led_state ? (1 << LEDC_DUTY_RESOLUTION) - 1 : 0;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    printf("Value: %" PRIu32 "\n", duty);
}

void led_toggle_switch() {
    led_toggle_stop();
    toggle();
}

void led_toggle_loop(uint64_t current_time) {
    if (!is_enabled) return;

    if (is_toggling) {
        // Toggle the LED every conf_toggle_usec
        if (current_time - last_toggle_time >= conf_toggle_usec) {
            toggle();
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


// // Stop the LEDC channel
// ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0)); // Stop LEDC

// // Resume the LEDC timer and channel
// ESP_ERROR_CHECK(ledc_timer_resume(LEDC_MODE, LEDC_TIMER)); // Resume timer
// ESP_ERROR_CHECK(ledc_channel_resume(LEDC_MODE, LEDC_CHANNEL)); // Resume channel

// // Configure the LEDC timer
// ledc_timer_config_t timer_config = {
//     .speed_mode = LEDC_MODE,
//     .duty_resolution = LEDC_DUTY_RESOLUTION,
//     .timer_num = LEDC_TIMER,
//     .freq_hz = LEDC_FREQUENCY,
//     .clk_cfg = LEDC_AUTO_CLK, // Automatic clock source
// };
// ESP_ERROR_CHECK(ledc_timer_config(&timer_config));

// // Configure LEDC channel 1 for GPIO 18
// ledc_channel_config_t channel_config_1 = {
//     .gpio_num = LEDC_GPIO_PIN_1,
//     .speed_mode = LEDC_MODE,
//     .channel = LEDC_CHANNEL_1,
//     .intr_type = LEDC_INTR_DISABLE, // No interrupt
//     .timer_sel = LEDC_TIMER,
//     .duty = 0, // Start with 0% duty cycle (LED off)
//     .hpoint = 0, // Phase point (default to 0)
// };
// ESP_ERROR_CHECK(ledc_channel_config(&channel_config_1));

// // Configure LEDC channel 2 for GPIO 19
// ledc_channel_config_t channel_config_2 = {
//     .gpio_num = LEDC_GPIO_PIN_2,
//     .speed_mode = LEDC_MODE,
//     .channel = LEDC_CHANNEL_2,
//     .intr_type = LEDC_INTR_DISABLE, // No interrupt
//     .timer_sel = LEDC_TIMER,
//     .duty = 0, // Start with 0% duty cycle (LED off)
//     .hpoint = 0, // Phase point (default to 0)
// };
// ESP_ERROR_CHECK(ledc_channel_config(&channel_config_2));