#ifndef SERV_COUNTER_H
#define SERV_COUNTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Function prototypes
typedef struct {
    uint16_t current_index;
    bool is_firstHalfCycle;
    uint8_t increment;
    uint16_t max_value;
    uint64_t refresh_time_uS;
    uint64_t last_refresh_time;
} cycleIndex_config_t;

void cycleIndex_check(
    uint64_t current_time,
    cycleIndex_config_t* obj, 
    void (*callback)(uint16_t current_index, bool is_firstHalfCycle)
);

typedef struct {
    int16_t current_value;
    bool is_increasing;
    int8_t increment;
    int16_t max_value;
    uint64_t refresh_time_uS;
    uint64_t last_refresh_time;
} cycleFade_config_t;

void cycleFade_check(
    uint64_t current_time,
    uint16_t obj_index,
    cycleFade_config_t* obj,
    void (*callback)(uint16_t index, int16_t current_value)
);

#endif