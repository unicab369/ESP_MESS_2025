#include "cycle_sequence.h"
#include <stdio.h>

// example sequence:
// value = 1 + increment
// value = 2 + increment
// value = 3 + increment    // max_value = 3, revert cycle
// value = 2 + increment
// value = 1 + increment

void cycle_fade(
    uint64_t current_time, uint16_t obj_index,
    sequence_config_t* conf,
    void (*callback)(uint16_t index, int16_t current_value)
) {
    // check refresh time
    if (current_time - conf->last_refresh_time < conf->refresh_time_uS) return;
    conf->last_refresh_time = current_time;

    int8_t increment = conf->increment;
    int16_t max_value = conf->max_value;
    int16_t current_value = conf->current_value;

    if (conf->is_switched) {
        conf->current_value += increment;

        // check when max value has been reached
        if (current_value > max_value) {
            conf->current_value = max_value;
            conf->is_switched = false;
        }
    } else {
        conf->current_value -= increment;

        // check when the min value has been reached
        if (current_value <= 0) {
            conf->current_value = 0;
            conf->is_switched = true;
        }
    }

    // call the callback
    callback(obj_index, conf->current_value);
}

static void handle_Switch() {

}

// example sequence:
// value = 0 + increment
// value = 1 + increment
// value = 2 + increment    // max_value = 2, reset cycle
// value = 0 + increment
// value = 1 + increment
// value = 2 + increment

void cycle_fill(
    uint64_t current_time, bool bouncing,
    step_sequence_config_t* conf, 
    step_sequence_cb callback
) {
    // check refresh time
    if (current_time - conf->last_refresh_time < conf->refresh_time_uS) return;
    conf->last_refresh_time = current_time;

    int8_t increment = conf->increment;
    int16_t max_value = conf->max_value;
    bool is_switched = conf->is_reversed;
    int16_t current_value = conf->current_value;
    
    // call the callback
    printf("curr = %d | %s\n", conf->current_value, is_switched ? "reverse" : "forward");
    callback(0, current_value, conf->previous_value, is_switched);
    conf->previous_value = conf->current_value;
    
    if (bouncing) {
        if (!is_switched) {
            conf->current_value += increment;
    
            if (conf->current_value >= max_value) {
                conf->current_value = max_value - 1;
                conf->is_reversed = true;
            }
        } else {
            conf->current_value -= increment;
            
            if (conf->current_value < 0) {
                conf->current_value = 0;
                conf->is_reversed = false;
            }
        }
    } else {
        // increase the index
        conf->current_value += increment;                           

        // check if max_count has been reached
        if (current_value >= max_value) {     
            conf->current_value = 0;                     
            conf->is_reversed = !is_switched;
        }
    }
}


void cycle_step(
    uint64_t current_time, bool bouncing, uint8_t index,
    step_sequence_config_t* conf,
    step_sequence_cb callback
) {
    // check refresh time
    if (current_time - conf->last_refresh_time < conf->refresh_time_uS) return;
    conf->last_refresh_time = current_time;

    int8_t increment = conf->increment;
    int16_t max_value = conf->max_value;
    bool is_reversed = conf->is_reversed;

    // call the callback
    // printf("curr = %d, prev = %d | %s\n", conf->current_value, conf->previous_value, is_reversed ? "reverse" : "forward");
    callback(index, conf->current_value, conf->previous_value, is_reversed);

    // update previous value
    conf->previous_value = conf->current_value;

    // note: bounce will neglect reverse
    if (bouncing) {
        if (!is_reversed) {
            conf->current_value += increment;

            // revert if reached max value
            if (conf->current_value >= max_value) {
                conf->current_value = max_value - 1;
                conf->is_reversed = true;
            }
        } else {
            conf->current_value -= increment;
    
            // revert if reached min value
            if (conf->current_value < 0) {
                conf->current_value = 0;
                conf->is_reversed = false;
            }
        }

    } else {
        if (!is_reversed) {
            // Forward movement
            conf->current_value = (conf->current_value + increment) % max_value;
    
            if (conf->current_value < increment) {
                conf->current_value = 0;
            }
        } else {
            // Backward movement
            conf->current_value = (conf->current_value - increment + max_value) % max_value;
    
            if (conf->current_value > max_value - increment) {
                conf->current_value = max_value - 1;
            }
        }
    }
}