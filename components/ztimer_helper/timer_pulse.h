#ifndef TIMER_PULSE_H
#define TIMER_PULSE_H

#include <stdint.h>
#include <stdbool.h>

// Function prototypes

typedef struct {
    uint8_t pulse_count;
    uint32_t pulse_time_ms;
    uint32_t wait_time_ms;
} timer_pulse_config_t;

typedef struct {
    int toggle_count;
    bool is_toggling;
    bool is_enabled;
    bool led_state;
    uint64_t last_wait_time;
    uint64_t last_toggle_time;

    struct {
        uint8_t half_cycle_count;
        uint32_t pulse_time_uS;
        uint32_t wait_time_uS;
    } config;
} timer_pulse_obj_t;

void timer_pulse_setup(timer_pulse_config_t config, timer_pulse_obj_t *obj);
void timer_pulse_reset(uint64_t current_time, timer_pulse_obj_t *obj);
void timer_pulse_handler(uint64_t current_time, timer_pulse_obj_t *obj, void (*callback)(void));

#endif