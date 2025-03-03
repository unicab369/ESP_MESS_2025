#include "serv_cycleIndex.h"


void serv_cycleIndex_check(
    uint64_t current_time,
    serv_cycleIndex_obj* obj, 
    void (*callback)(uint16_t current_index, bool is_firstHalfCycle)
) {
    // check refresh time
    if (current_time - obj->last_refresh_time < obj->refresh_time_uS) return;
    obj->last_refresh_time = current_time;

    // call the callback
    callback(obj->current_index, obj->is_firstHalfCycle);

    // increase the index
    obj->current_index += obj->increment;                           

    // check if max_count has been reached
    if (obj->current_index >= obj->max_value) {     
        // reset index and toggle counting
        obj->current_index = 0;                     
        obj->is_firstHalfCycle = !obj->is_firstHalfCycle;
    }
}

void serv_cycleFade_check(
    uint64_t current_time,
    serv_cycleFade_obj* obj,
    void (*callback)(uint16_t current_value)
) {
    // check refresh time
    if (current_time - obj->last_refresh_time < obj->refresh_time_uS) return;
    obj->last_refresh_time = current_time;

    if (obj->is_increasing) {
        obj->current_value += obj->increment;

        // check when max value has been reached
        if (obj->current_value > obj->max_value) {
            obj->current_value = obj->max_value;
            obj->is_increasing = false;
        }
    } else {
        obj->current_value -= obj->increment;

        // check when the min value hsa been reached
        if (obj->current_value <= 0) {
            obj->current_value = 0;
            obj->is_increasing = true;
        }
    }

    // call the callback
    callback(obj->current_value);
}