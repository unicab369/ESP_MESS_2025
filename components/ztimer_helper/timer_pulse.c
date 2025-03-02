#include "timer_pulse.h"

// static int toggle_count = 0;
// static bool is_toggling = false;
// static bool is_enabled = false;
// static uint64_t last_wait_time = 0;
// static uint64_t last_toggle_time = 0;

// static uint8_t pulse_count_max = 1;
// static uint16_t pulse_time_usec = 100000;       // 200 ms
// static uint16_t wait_time_usec = 1000000;       // 1000 ms

// void timer_pulse_setup(uint8_t pulse_count, uint16_t pulse_time_ms, uint16_t wait_time_ms) {
//     pulse_count_max = pulse_count*2;
//     pulse_time_usec = pulse_time_ms*1000;
//     wait_time_usec = wait_time_ms*1000;
// }

void timer_pulse_setup(timer_pulse_config_t config, timer_pulse_obj_t *obj) {
    obj->config.half_cycle_count = config.pulse_count*2;
    obj->config.pulse_time_uS = config.pulse_time_ms*1000;
    obj->config.wait_time_uS = config.wait_time_ms*1000;
}

void timer_pulse_reset(uint64_t current_time, timer_pulse_obj_t *obj) {
    // reset values
    obj->toggle_count = 0;
    obj->is_toggling = true;
    obj->is_enabled = true;
    obj->last_wait_time = current_time;
    obj->last_toggle_time = current_time;
}

void timer_pulse_handler(uint64_t current_time, timer_pulse_obj_t *obj, void (*callback)(void)) {
    if (!obj->is_enabled) return;

    if (obj->is_toggling) {
        // pulse time
        if (current_time - obj->last_toggle_time >= obj->config.pulse_time_uS) {
            obj->last_toggle_time = current_time;
            obj->toggle_count++;
            callback();

            // If we've toggled enough times, switch to waiting
            if (obj->toggle_count >= obj->config.half_cycle_count) {
                obj->is_toggling = false;
                obj->toggle_count = 0;
                obj->last_wait_time = current_time;
            }
        }
    } else {
        // wait time for starting toggling again
        if (current_time - obj->last_wait_time >= obj->config.wait_time_uS) {
            obj->is_toggling = true;
            obj->last_toggle_time = current_time;
        }
    }
}