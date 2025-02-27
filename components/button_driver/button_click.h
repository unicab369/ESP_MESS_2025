#ifndef BUTTON_DETECTOR_H
#define BUTTON_DETECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>

// Button event types
typedef enum {
    BUTTON_EVENT_SINGLE_CLICK,
    BUTTON_EVENT_DOUBLE_CLICK,
    BUTTON_EVENT_LONG_PRESS
} button_event_t;

// Callback function type
typedef void (*button_event_callback_t)(button_event_t event, uint64_t duration);

// Function prototypes
void button_click_setup(gpio_num_t button_gpio, button_event_callback_t callback);
void button_click_run(void);

#endif