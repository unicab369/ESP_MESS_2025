#ifndef ROTARY_DRIVER_H
#define ROTARY_DRIVER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Callback function
typedef void (*rotary_callback_t)(int16_t value, bool direction);

// Function prototypes
void rotary_setup(uint8_t clk_pin, uint8_t dt_pin, rotary_callback_t callback);
void rotary_loop(uint64_t current_time);

#endif