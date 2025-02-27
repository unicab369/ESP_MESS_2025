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
} gpio_toggle_t;

// Function prototypes
void led_toggle_setup(gpio_num_t gpio);
void led_toggle_restart(void);
void led_toggle_pulses(uint8_t count, uint32_t repeat_duration);

void led_toggle_switch(void);
void led_toggle_value(bool onOff);
void led_toggle_off(void);
void led_toggle_loop(void);

#endif // LED_TOGGLE_H