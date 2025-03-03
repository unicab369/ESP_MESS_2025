#include "serv_cycleIndex.h"


void serv_cycleIndex_check(
    uint64_t current_time,
    serv_cycleIndex_obj* obj, 
    void (*callback)(uint16_t current_index, bool is_firstHalfCycle)
) {
    if (current_time - obj->last_refresh_time < obj->refresh_time_uS) return;
     // track the last_refresh_time
    obj->last_refresh_time = current_time;

    // call the callback
    callback(obj->current_index, obj->is_firstHalfCycle);

    // increase the index
    obj->current_index++;                           

    // check if max_count has been reached
    if (obj->current_index >= obj->max_count) {     
        // reset index and toggle counting
        obj->current_index = 0;                     
        obj->is_firstHalfCycle = !obj->is_firstHalfCycle;
    }
}

