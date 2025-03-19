#include "mod_utility.h"


uint16_t map_value(uint16_t x, uint16_t input_min, uint16_t input_max, 
    uint16_t output_min, uint16_t output_max) {
    uint16_t diff = (x - input_min) * (output_max - output_min);
    uint16_t output = output_min + diff / (input_max - input_min);

    if (output > output_max) output = output_max;
    if (output < output_min) output = output_min;
    return output;
}

int constrain_value(int value, int low, int high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}
