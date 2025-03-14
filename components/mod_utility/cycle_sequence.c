#include "cycle_sequence.h"
#include <stdio.h>

// example sequence:
void cycle_values(
    uint64_t current_time, uint8_t obj_index,
    step_sequence_config_t* conf, 
    sequence_cb callback
) {
    // check refresh time
    if (current_time - conf->last_refresh_time < conf->refresh_time_uS) return;
    conf->last_refresh_time = current_time;

    // call the callback
    callback(obj_index, conf);
    conf->previous_value = conf->current_value;

    bool isToggled = conf->is_toggled;
    bool direction = conf->direction;
    int8_t increment = conf->increment;
    int16_t startIndex = conf->start_index;
    int16_t endIndex = conf->end_index;
    // printf("curr = %d | toggled = %s\n", conf->current_value, isToggled ? "true" : "false");
    
    if (conf->is_bounced) {
        //! bouncing
        // filling cycle is not uniform because the half cycles are not the same
        // this value affects by is_bounced only

        if (direction) {
            int16_t endOffset = conf->is_uniformed ? -1 : 0;
            conf->current_value += increment;
    
            // check limit
            if (conf->current_value > endIndex + endOffset) {
                conf->current_value = endIndex;
                conf->direction = !direction;       // switch direction
                conf->is_toggled = !isToggled;      // switch toggle
            }
        } else {
            uint16_t startOffset = conf->is_uniformed ? 1 : 0;
            conf->current_value -= increment;
            
            // check limit
            if (conf->current_value < startIndex + startOffset) {
                conf->current_value = startIndex;
                conf->direction = !direction;       // switch direction
                conf->is_toggled = !isToggled;      // switch toggle
            }
        }

    } else {
        //! no bouncing
        if (direction) {
            conf->current_value += increment;                           

            // check limit
            if (conf->current_value > endIndex) {  
                conf->current_value = startIndex;                     
                conf->is_toggled = !isToggled;          // switch toggle
            }
        } else {
            conf->current_value -= increment;                           

            // check limit
            if (conf->current_value < startIndex) {     
                conf->current_value = endIndex;                     
                conf->is_toggled = !isToggled;          // switch toggle
            }
        }
    }
}
