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

void fill_color_byValue(RGB_t* input, int16_t value);
RGB_t make_color_byChannels(int16_t value, RGB_t channels);

void hsv_to_rgb(float h, float s, float v, RGB_t* rgb);
void hsv_to_rgb_ints(uint8_t hue, uint8_t sat, uint8_t value, RGB_t* rgb);

#endif