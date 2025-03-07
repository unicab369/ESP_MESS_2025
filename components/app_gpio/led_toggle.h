#ifndef LED_TOGGLE_H
#define LED_TOGGLE_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>
#include <stddef.h>
#include "timer_pulse.h"


typedef struct {
    uint8_t object_index;
    int blink_count;         // Number of times to blink the LED
    uint32_t toggle_time_ms; // Time (in milliseconds) to toggle the LED on/off
    uint16_t wait_time_ms;   // Time (in milliseconds) to wait between blinks
    bool invert_state;
    timer_pulse_config_t timer_config;
} gpio_toggle_obj;

// Function prototypes
void gpio_digital_setup(gpio_num_t gpio);
void led_toggle_restart(void);
void gpio_digital_config(gpio_toggle_obj object);

void gpio_digital_set(bool onOff);
void gpio_digital_toggle(void);

void gpio_digital_stop(uint8_t index);
void gpio_digital_loop(uint64_t current_time);

#endif // LED_TOGGLE_H