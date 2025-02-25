#ifndef LED_FADE_H
#define LED_FADE_H

#include <stdint.h>
#include <soc/gpio_num.h>
#include "driver/ledc.h"

// Function prototypes
void led_fade_init(gpio_num_t led_gpio, uint32_t freq_hz);
void led_fade_start(uint32_t brightness_threshold, uint32_t toggle_duration_ms, uint32_t update_frequency_ms);
void led_fade_update(void);

#endif // LED_FADE_H