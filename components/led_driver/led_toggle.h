#ifndef LED_TOGGLE_H
#define LED_TOGGLE_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>

typedef struct {
    int blink_count;         // Number of times to blink the LED
    uint32_t toggle_time_ms; // Time (in milliseconds) to toggle the LED on/off
    uint32_t wait_time_ms;   // Time (in milliseconds) to wait between blinks
    bool invert_state;
} led_config_t;

// Function prototypes
void led_toggle_setup(gpio_num_t gpio, led_config_t config);
void led_toggle_run(void);

#endif // LED_TOGGLE_H