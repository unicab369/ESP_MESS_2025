#include <math.h>
#include "color_helper.h"

RGB_t set_color_byChannels(int16_t value, RGB_t channels) {
    return (RGB_t){
        .red = channels.red & value,
        .green = channels.green & value,
        .blue = channels.blue & value
    };
}

void hsv_to_rgb(float h, float s, float v, RGB_t* rgb) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    
    float r_temp, g_temp, b_temp;
    if (h < 60) {
        r_temp = c; g_temp = x; b_temp = 0;
    } else if (h < 120) {
        r_temp = x; g_temp = c; b_temp = 0;
    } else if (h < 180) {
        r_temp = 0; g_temp = c; b_temp = x;
    } else if (h < 240) {
        r_temp = 0; g_temp = x; b_temp = c;
    } else if (h < 300) {
        r_temp = x; g_temp = 0; b_temp = c;
    } else {
        r_temp = c; g_temp = 0; b_temp = x;
    }
    
    rgb->red = (uint8_t)((r_temp + m) * 255);
    rgb->green = (uint8_t)((g_temp + m) * 255);
    rgb->blue = (uint8_t)((b_temp + m) * 255);
}
