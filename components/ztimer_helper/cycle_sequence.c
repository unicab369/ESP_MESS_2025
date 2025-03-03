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

static void handle_switch_direction(step_sequence_config_t* conf) {
    if (!conf->direction) {
        conf->current_value += conf->increment;

        if (conf->current_value >= conf->max_value) {
            conf->current_value = conf->max_value - 1;
            conf->direction = true;
            conf->is_toggled = false;
        }
    } else {
        conf->current_value -= conf->increment;
        
        if (conf->current_value < 0) {
            conf->current_value = 0;
            conf->direction = false;
            conf->is_toggled = true;
        }
    }
}

// example sequence:
// value = 0 + increment
// value = 1 + increment
// value = 2 + increment    // max_value = 2, reset cycle
// value = 0 + increment
// value = 1 + increment
// value = 2 + increment

void cycle_fill(
    uint64_t current_time, uint8_t obj_index,
    step_sequence_config_t* conf, 
    sequence_cb callback
) {
    // check refresh time
    if (current_time - conf->last_refresh_time < conf->refresh_time_uS) return;
    conf->last_refresh_time = current_time;

    int8_t increment = conf->increment;
    int16_t max_value = conf->max_value;
    bool direction = conf->direction;

    // call the callback
    printf("curr = %d | %s\n", conf->current_value, direction ? "inverted" : "normal");
    callback(obj_index, conf);
    conf->previous_value = conf->current_value;
    
    // check for bouncing
    if (conf->is_bounced) {
        // revert direction if reaches min or max
        handle_switch_direction(conf);

    } else {
        // reverse if reaches min or max
        if (direction) {
            // normal
            conf->current_value += increment;                           

            // check if max value has been reached
            if (conf->current_value >= max_value) {     
                conf->current_value = 0;                     
                conf->is_toggled = !conf->is_toggled;
            }
        } else {
            conf->current_value -= increment;                           

            // check if min value has been reached
            if (conf->current_value < 0) {     
                conf->current_value = max_value - 1;                     
                conf->is_toggled = !conf->is_toggled;
            }
        }
    }
}


void cycle_step(
    uint64_t current_time, uint8_t obj_index,
    step_sequence_config_t* conf,
    sequence_cb callback
) {
    // check refresh time
    if (current_time - conf->last_refresh_time < conf->refresh_time_uS) return;
    conf->last_refresh_time = current_time;

    int8_t increment = conf->increment;
    int16_t max_value = conf->max_value;
    bool direction = conf->direction;

    // call the callback
    // printf("curr = %d, prev = %d | %s\n", conf->current_value, conf->previous_value, direction ? "reverse" : "forward");
    callback(obj_index, conf);

    // update previous value
    conf->previous_value = conf->current_value;

    // check for bouncing
    if (conf->is_bounced) {
        // revert direction if reaches min or max
        handle_switch_direction(conf);

    } else {
        if (direction) {
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