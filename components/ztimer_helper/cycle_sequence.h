#ifndef SERV_COUNTER_H
#define SERV_COUNTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Function prototypes
typedef struct {
    int16_t current_value;
    int16_t previous_value;
    bool is_switched;
    int8_t increment;
    int16_t max_value;
    uint64_t refresh_time_uS;
    uint64_t last_refresh_time;
} sequence_config_t;

typedef struct {
    int16_t current_value;
    int16_t previous_value;
    bool direction;
    bool is_bounced;
    bool is_toggled;        // toggled when reaches max or min
    int8_t increment;
    int16_t max_value;
    uint64_t refresh_time_uS;
    uint64_t last_refresh_time;
} step_sequence_config_t;


typedef void (*sequence_cb)(uint8_t index, step_sequence_config_t* conf);

void cycle_indexes(
    uint64_t current_time, uint8_t obj_index,
    step_sequence_config_t* obj, 
    sequence_cb callback
);

void cycle_fade(uint64_t current_time, uint16_t obj_index,
    sequence_config_t* obj, void (*callback)(uint16_t index, int16_t current_value)
);

#endif