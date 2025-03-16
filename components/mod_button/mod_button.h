#ifndef BUTTON_DETECTOR_H
#define BUTTON_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>

// Button event types
typedef enum __attribute__((packed)) {
    BUTTON_SINGLE_CLICK = 0x01,
    BUTTON_DOUBLE_CLICK = 0x02,
    BUTTON_LONG_PRESS = 0x00,
} button_event_t;

// Callback function type
typedef void (*button_event_callback_t)(button_event_t event, uint8_t pin, uint64_t duration);

// Function prototypes
void button_click_setup(gpio_num_t button_gpio, button_event_callback_t callback);
void button_click_loop(uint64_t current_time);

#endif