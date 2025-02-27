#include "led_fade.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

static ledc_channel_t led_channel;
static uint32_t brightness_threshold;
static uint32_t update_frequency_us;
static uint32_t current_duty = 0;
static bool is_fading_up = true;
static uint64_t last_update_time = 0;
static uint32_t fade_duration_us = 2000000;


void led_fade_setup(gpio_num_t led_gpio, uint32_t freq_hz) {
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = freq_hz,
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
    led_channel = LEDC_CHANNEL_0;
}

void led_fade_start(uint32_t threshold, uint32_t duration_ms, uint32_t frequency_ms) {
    brightness_threshold = threshold;
    update_frequency_us = frequency_ms*1000;
    fade_duration_us = duration_ms*1000;
    last_update_time = esp_timer_get_time();
}

void led_fade_loop(void) {
    uint64_t current_time = esp_timer_get_time();

    if (current_time - last_update_time >= update_frequency_us) {
        last_update_time = current_time;

        //Calculate the step size based on fade duration and threshold
        uint32_t step_size = brightness_threshold / (fade_duration_us / update_frequency_us); 
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

        ledc_set_duty(LEDC_LOW_SPEED_MODE, led_channel, current_duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, led_channel);
    }
}