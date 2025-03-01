#include "led_fade.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#define LED_UPDATE_FREQUENCY 10000     // 10ms

static uint32_t brightness_threshold;
static uint32_t update_frequency_us;
static uint32_t current_duty = 0;
static bool is_fading_up = true;
static uint64_t last_update_time = 0;
static uint32_t fade_duration_us = 2000000;
static uint8_t led_gpio;
static bool is_enabled = false;

void led_fade_setup(gpio_num_t gpio) {
    led_gpio = gpio;

    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t channel_conf = {
        .gpio_num = led_gpio,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_conf);
}

void led_fade_restart(uint32_t threshold, uint32_t duration_ms) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);

    brightness_threshold = threshold;
    fade_duration_us = duration_ms*1000;
    last_update_time = esp_timer_get_time();
    is_enabled = true;
}

void led_fade_stop() {
    //! stop PWM
    // ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    is_enabled = false;
}

void led_fade_loop(void) {
    uint64_t current_time = esp_timer_get_time();

    if (is_enabled && current_time - last_update_time >= LED_UPDATE_FREQUENCY) {
        last_update_time = current_time;

        //Calculate the step size based on fade duration and threshold
        uint32_t step_size = brightness_threshold / (fade_duration_us / LED_UPDATE_FREQUENCY); 
        if(step_size == 0) step_size = 1; //Ensure step size is not zero

        if (is_fading_up) {
            current_duty += step_size;
            if (current_duty >= brightness_threshold) {
                current_duty = brightness_threshold;
                is_fading_up = false;
            }
        } else {
            if (current_duty >= step_size)
                current_duty -= step_size;
            else
               current_duty = 0;

            if (current_duty == 0) {
                is_fading_up = true;
            }
        }

        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, current_duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
}