#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <soc/gpio_num.h>
#include "driver/rmt_tx.h"

#include "timer_pulse.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)

static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 1,
    .duration0 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0H=0.3us
    .level1 = 0,
    .duration1 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T0L=0.9us
};

static const rmt_symbol_word_t ws2812_one = {
    .level0 = 1,
    .duration0 = 0.9 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1H=0.9us
    .level1 = 0,
    .duration1 = 0.3 * RMT_LED_STRIP_RESOLUTION_HZ / 1000000, // T1L=0.3us
};

//reset defaults to 50uS
static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 1,
    .duration0 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
    .level1 = 0,
    .duration1 = RMT_LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2,
};

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} ws2812_rgb_t;


typedef struct {
    uint8_t led_idx;
    uint8_t pulse_idx;
    ws2812_rgb_t rgb;
    timer_pulse_config_t config;
    void (*pulse_cb)(void);
} ws2812_pulse_obj_t;


void ws2812_setup(void);
void ws2812_load_obj(ws2812_pulse_obj_t objx) ;

void ws2812_toggle(bool state, uint8_t led_index);
void ws2812_run1(uint64_t current_time);
void ws2812_loop(uint64_t current_time);
void ws2812_loop2(uint64_t current_time);

#endif