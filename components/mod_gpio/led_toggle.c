#include "led_toggle.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

#define LEDC_DUTY_RESOLUTION   LEDC_TIMER_10_BIT

static const char *TAG = "GPIO_TOGGLE";

static gpio_num_t led_gpio;

#define PULSE_OJB_COUNT1 1
static timer_pulse_obj_t timer_objs[PULSE_OJB_COUNT1];
static gpio_toggle_obj toggle_objs[PULSE_OJB_COUNT1];

void gpio_digital_setup(gpio_num_t gpio) {
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

void gpio_digital_stop(uint8_t index) {
    timer_objs[index].is_enabled = false; // stop
}

void gpio_digital_config(gpio_toggle_obj object) {
    toggle_objs[object.object_index] = object;

    timer_pulse_obj_t* timer_obj = &timer_objs[object.object_index];
    timer_pulse_setup(object.timer_config, timer_obj);
    timer_pulse_reset(esp_timer_get_time(), timer_obj);
}

// void gpio_digital_set(bool onOff) {
//     pulse_obj.is_enabled = false;       // stop
//     gpio_set_level(led_gpio, onOff);
// }

// void gpio_digital_toggle() {
//     pulse_obj.is_enabled = false;       // stop
//     toggle(pulse_obj.current_state);
// }

static void pulse_handler(uint8_t index, bool state) {
    // gpio_set_level(led_gpio, led_state);
    uint32_t duty = state ? (1 << LEDC_DUTY_RESOLUTION) - 1 : 0;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void gpio_digital_loop(uint64_t current_time) {
    for (int i=0; i<PULSE_OJB_COUNT1; i++) {
        timer_pulse_obj_t* obj = &timer_objs[i];
        timer_pulse_handler(current_time, i, obj, pulse_handler);
    }
}









//! stop PWM
// ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);

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