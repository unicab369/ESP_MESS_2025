#ifndef TIMER_PULSE_H
#define TIMER_PULSE_H

#include <stdint.h>
#include <stdbool.h>

// Function prototypes

typedef struct {
    uint8_t pulse_count;
    uint16_t pulse_time_ms;
    uint16_t wait_time_ms;
} timer_pulse_config_t;

typedef struct {
    int toggle_count;
    bool is_toggling;
    bool is_enabled;
    uint64_t last_wait_time;
    uint64_t last_toggle_time;

    struct {
        uint8_t half_cycle_count;
        uint16_t pulse_time_uS;
        uint16_t wait_time_uS;
    } config;
} timer_pulse_obj_t;

#endif