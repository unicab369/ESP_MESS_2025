#ifndef COLOR_HELPER_H
#define COLOR_HELPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Function prototypes
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB_t;

static RGB_t rgb_off = {
    .red = 0, .green = 0, .blue = 0
};

RGB_t set_color_byChannels(int16_t value, RGB_t channels);
void hsv_to_rgb(float h, float s, float v, RGB_t* rgb);
void hsv_to_rgb_ints(uint8_t hue, uint8_t sat, uint8_t value, RGB_t* rgb);

#endif