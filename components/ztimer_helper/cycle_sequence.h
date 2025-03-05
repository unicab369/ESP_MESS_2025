#ifndef SERV_COUNTER_H
#define SERV_COUNTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int16_t current_value;
    int16_t previous_value;
    bool direction;
    bool is_bounced;
    bool is_toggled;        // toggled when reaches max or min

    // filling cycle is not uniform because the half cycles are not the same
    // this value affects by is_bounced only
    bool is_uniformed;
    int8_t increment;
    int16_t start_index;
    int16_t end_index;
    uint64_t refresh_time_uS;
    uint64_t last_refresh_time;
} step_sequence_config_t;


typedef void (*sequence_cb)(uint8_t index, step_sequence_config_t* conf);

void cycle_values(
    uint64_t current_time, uint8_t obj_index,
    step_sequence_config_t* obj, 
    sequence_cb callback
);

#endif