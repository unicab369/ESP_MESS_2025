#ifndef LED_TOGGLE_H
#define LED_TOGGLE_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>
#include <stddef.h>
#include "timer_pulse.h"


typedef struct {
    int blink_count;         // Number of times to blink the LED
    uint32_t toggle_time_ms; // Time (in milliseconds) to toggle the LED on/off
    uint16_t wait_time_ms;   // Time (in milliseconds) to wait between blinks
    bool invert_state;
} gpio_toggle_t;

// Function prototypes
void led_toggle_setup(gpio_num_t gpio);
void led_toggle_restart(void);
void led_toggle_pulses(timer_pulse_config_t config, timer_pulse_obj_t* object);
void led_toggle(bool state);

void led_toggle_switch(void);
void led_toggle_setValue(bool onOff);
void led_toggle_stop(timer_pulse_obj_t* object);
void led_toggle_loop(uint64_t current_time, timer_pulse_obj_t* objects, size_t len);

#endif // LED_TOGGLE_H