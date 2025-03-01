#ifndef LED_FADE_H
#define LED_FADE_H

#include <stdint.h>
#include <soc/gpio_num.h>
#include "driver/ledc.h"

// Function prototypes
void led_fade_setup(gpio_num_t led_gpio);
void led_fade_restart(uint32_t brightness_threshold, uint32_t toggle_duration_ms);
void led_fade_stop();
void led_fade_loop(uint64_t current_time);

#endif // LED_FADE_H