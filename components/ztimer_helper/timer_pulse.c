#include "timer_pulse.h"
#include <strings.h>

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
    obj->current_state = false;
    obj->last_wait_time = current_time;
    obj->last_toggle_time = current_time;
}

void timer_pulse_handler(
    uint64_t current_time, timer_pulse_obj_t *objects, size_t len, 
    void (*callback)(uint8_t index, bool state)
) {
    for (int i=0; i<len; i++) {
        timer_pulse_obj_t* obj = &objects[i];
        if (!obj->is_enabled) continue;

        if (obj->is_toggling) {
            // pulse time
            if (current_time - obj->last_toggle_time >= obj->config.pulse_time_uS) {
                obj->last_toggle_time = current_time;
                obj->toggle_count++;
                obj->current_state = !obj->current_state;
                callback(i, obj->current_state);
    
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

}
