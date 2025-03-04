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


// Function to convert HSV to RGB
void hsv_to_rgb_ints(uint8_t hue, uint8_t sat, uint8_t value, RGB_t* rgb) {
    uint8_t region = hue / 43;
    uint8_t remainder = (hue % 43) * 6;
    uint8_t p = (value * (255 - sat)) >> 8;   // >> 8 is same as / 255
    uint8_t q = (value * (255 - ((sat * remainder) >> 8))) >> 8;
    uint8_t t = (value * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:     rgb->red = value;   rgb->green = t;         rgb->blue = p;      break;
        case 2:     rgb->red = p;       rgb->green = value;     rgb->blue = t;      break;
        case 1:     rgb->red = q;       rgb->green = value;     rgb->blue = p;      break;
        case 3:     rgb->red = p;       rgb->green = q;         rgb->blue = value;  break;
        case 4:     rgb->red = t;       rgb->green = p;         rgb->blue = value;  break;
        default:    rgb->red = value;   rgb->green = p;         rgb->blue = q;      break;
    }
}
