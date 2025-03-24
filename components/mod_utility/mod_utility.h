#define _ENUM_TO_STR(name)      #name
#define ENUM_TO_STR(name)       _ENUM_TO_STR(name)

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

#include <unistd.h>

uint16_t map_value(uint16_t x, uint16_t input_min, uint16_t input_max, 
    uint16_t output_min, uint16_t output_max);

int constrain_value(int value, int low, int high);

void print_hex_debug(uint16_t value);