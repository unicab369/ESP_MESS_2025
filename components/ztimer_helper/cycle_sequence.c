#include "cycle_sequence.h"


// example sequence:
// value = 0 + increment
// value = 1 + increment
// value = 2 + increment    // max_value = 2, reset cycle
// value = 0 + increment
// value = 1 + increment
// value = 2 + increment

void cycleIndex_check(
    uint64_t current_time,
    sequence_config_t* obj, 
    void (*callback)(int16_t current_value, bool is_switched)
) {
    // check refresh time
    if (current_time - obj->last_refresh_time < obj->refresh_time_uS) return;
    obj->last_refresh_time = current_time;

    // call the callback
    callback(obj->current_value, obj->is_switched);

    // increase the index
    obj->current_value += obj->increment;                           

    // check if max_count has been reached
    if (obj->current_value >= obj->max_value) {     
        // reset index and toggle counting
        obj->current_value = 0;                     
        obj->is_switched = !obj->is_switched;
    }
}

// example sequence:
// value = 1 + increment
// value = 2 + increment
// value = 3 + increment    // max_value = 3, revert cycle
// value = 2 + increment
// value = 1 + increment

void cycleFade_check(
    uint64_t current_time,
    uint16_t obj_index,
    sequence_config_t* obj,
    void (*callback)(uint16_t index, int16_t current_value)
) {
    // check refresh time
    if (current_time - obj->last_refresh_time < obj->refresh_time_uS) return;
    obj->last_refresh_time = current_time;

    if (obj->is_switched) {
        obj->current_value += obj->increment;

        // check when max value has been reached
        if (obj->current_value > obj->max_value) {
            obj->current_value = obj->max_value;
            obj->is_switched = false;
        }
    } else {
        obj->current_value -= obj->increment;

        // check when the min value has been reached
        if (obj->current_value <= 0) {
            obj->current_value = 0;
            obj->is_switched = true;
        }
    }

    // call the callback
    callback(obj_index, obj->current_value);
}

void cycleStep_check(

) {

}