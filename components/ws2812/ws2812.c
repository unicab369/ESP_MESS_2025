#include "ws2812.h"
#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "serv_cycleIndex.c"

#define RMT_LED_STRIP_GPIO_NUM      12
#define EXAMPLE_LED_NUMBERS         7

#define EXAMPLE_FRAME_DURATION_MS   20
#define EXAMPLE_ANGLE_INC_FRAME     0.02
#define EXAMPLE_ANGLE_INC_LED       0.3

static const char *TAG = "example";

static uint8_t led_pixels[EXAMPLE_LED_NUMBERS * 3];


static size_t encoder_callback(const void *data, size_t data_size,
                               size_t symbols_written, size_t symbols_free,
                               rmt_symbol_word_t *symbols, bool *done, void *arg)
{
    // We need a minimum of 8 symbol spaces to encode a byte. We only
    // need one to encode a reset
    if (symbols_free < 8) {
        return 0;
    }

    // We can calculate where in the data we are from the symbol pos.
    size_t data_pos = symbols_written / 8;
    uint8_t *data_bytes = (uint8_t*)data;

    if (data_pos < data_size) {
        // Encode a byte
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
            uint8_t check = data_bytes[data_pos]&bitmask;
            symbols[symbol_pos++] = check ? ws2812_one : ws2812_zero;
        }
        // We're done; we should have written 8 symbols.
        return symbol_pos;
    } else {
        //All bytes already are encoded. Encode the reset, and we're done.
        symbols[0] = ws2812_reset;
        *done = 1; //Indicate end of the transaction.
        return 1; //we only wrote one symbol
    }
}

rmt_channel_handle_t led_chan = NULL;
rmt_encoder_handle_t simple_encoder = NULL;

const rmt_simple_encoder_config_t simple_encoder_cfg = {
    .callback = encoder_callback
    //Note we don't set min_chunk_size here as the default of 64 is good enough.
};

rmt_transmit_config_t tx_config = {
    .loop_count = 0, // no transfer loop
};


#define WS2812_TRANSMIT_FREQUENCY 30000   // micro seconds

#define PULSE_OJB_COUNT2 2
static timer_pulse_obj_t timer_objs[PULSE_OJB_COUNT2];
static ws2812_pulse_obj_t ws2812_pulse_objs[PULSE_OJB_COUNT2];

uint64_t last_transmit_time;

static uint64_t last_moved_time;
bool is_filling = false;

void ws2812_setup(void) {
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // number of transactions that can be pending in the background
    };
    rmt_new_tx_channel(&tx_chan_config, &led_chan);
    rmt_new_simple_encoder(&simple_encoder_cfg, &simple_encoder);
    rmt_enable(led_chan);
}

void ws2812_load_pulse(ws2812_pulse_obj_t object) {
    last_transmit_time = esp_timer_get_time();
    ws2812_pulse_objs[object.obj_index] = object;

    timer_pulse_obj_t* timer_obj = &timer_objs[object.obj_index];
    timer_pulse_setup(object.config, timer_obj);
    timer_pulse_reset(esp_timer_get_time(), timer_obj);
}

static void reset_leds(bool transmit) {
    memset(led_pixels, 0, sizeof(led_pixels));
    if (!transmit) return;

    rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
}

static void request_update_leds(uint16_t index, ws2812_rgb_t rgb) {
    led_pixels[index * 3 + 0] = rgb.green;        // Green
    led_pixels[index * 3 + 1] = rgb.red;          // Red
    led_pixels[index * 3 + 2] = rgb.blue;         // Blue
}

static void on_pulse_handler(uint8_t index, bool state) {
    ws2812_pulse_obj_t* obj = &ws2812_pulse_objs[index];
    ws2812_rgb_t output = state ? obj->rgb : rgb_off;
    request_update_leds(obj->led_index, output);
}

int led_index = 0;

//! Cycle Index
serv_cycleIndex_obj cycleIndex_obj = {
    .current_index = 0,
    .is_firstHalfCycle = true,
    .increment = 1,
    .max_value = 7,
    .refresh_time_uS = 60000,
    .last_refresh_time = 0
};

void on_cycleIndex_cb(uint16_t index, bool is_firstHalfCycle) {
    ws2812_rgb_t rgb_on = { .red = 0, .green = 0, .blue = 150 };
    ws2812_rgb_t value = is_firstHalfCycle ? rgb_on : rgb_off;
    request_update_leds(index, value);
}

//! Cycle Fade
serv_cycleFade_obj cycleFade_obj = {
    .current_value = 0,
    .is_increasing = true,
    .increment = 7,
    .max_value = 200,
    .refresh_time_uS = 20000,
    .last_refresh_time = 0
};

void onCycleFade_cb(uint16_t fading_value) {
    ws2812_rgb_t rgb_on = { .red = fading_value, .green = 0, .blue = fading_value };
    request_update_leds(2, rgb_on);
}

void ws2812_loop(uint64_t current_time) {
    //! handle pulses
    for (int i=0; i<PULSE_OJB_COUNT2; i++) {
        timer_pulse_obj_t* obj = &timer_objs[i];
        timer_pulse_handler(current_time, i, obj, on_pulse_handler);
    }

    //! handle moving leds
    // serv_cycleIndex_check(current_time, &cycleIndex_obj, on_cycleIndex_cb);

    //! handle fading leds
    serv_cycleFade_check(current_time, &cycleFade_obj, onCycleFade_cb);

    //! transmit the updated leds
    if (current_time - last_transmit_time < WS2812_TRANSMIT_FREQUENCY) return;
    last_transmit_time = current_time;
    rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
}


float offset = 0;

void ws2812_run1(uint64_t current_time) {
    for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
        // Build RGB pixels. Each color is an offset sine, which gives a
        // hue-like effect.
        float angle = offset + (led * EXAMPLE_ANGLE_INC_LED);
        const float color_off = (M_PI * 2) / 3;
        led_pixels[led * 3 + 0] = sin(angle + color_off * 0) * 127 + 128;
        led_pixels[led * 3 + 1] = sin(angle + color_off * 1) * 127 + 128;
        led_pixels[led * 3 + 2] = sin(angle + color_off * 2) * 117 + 128;;
    }
    // Flush RGB values to LEDs
    rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
    rmt_tx_wait_all_done(led_chan, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(EXAMPLE_FRAME_DURATION_MS));
    //Increase offset to shift pattern
    offset += EXAMPLE_ANGLE_INC_FRAME;
    if (offset > 2 * M_PI) {
        offset -= 2 * M_PI;
    }
}

#define MOVE_FORWARD 0
#define MOVE_BACKWARD 1

static int led_direction = MOVE_FORWARD;

int skip_pixels = 0;
ws2812_rgb_t rgb_value = { .red = 200, .green = 0, .blue = 0 };
ws2812_rgb_t off_value = { .red = 0, .green = 0, .blue = 0 };

// void ws2812_loop(uint64_t current_time) {
//     // Clear all LEDs
//     memset(led_strip_pixels, 0, sizeof(led_strip_pixels));

//     // Turn on the current LED
//     request_update_leds(led_index, rgb_value);
//     rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);

//     // Delay before moving to the next LED
//     vTaskDelay(pdMS_TO_TICKS(100));  // Adjust this value to change the speed of movement

//     // Move to the next LED position
//     if (led_direction == MOVE_FORWARD) {
//         led_index = (led_index + 1 + skip_pixels) % EXAMPLE_LED_NUMBERS;
//         if (led_index < skip_pixels) {
//             led_direction = MOVE_BACKWARD;
//             led_index = EXAMPLE_LED_NUMBERS - 1;
//         }
//     } else {
//         led_index = (led_index - 1 - skip_pixels + EXAMPLE_LED_NUMBERS) % EXAMPLE_LED_NUMBERS;
//         if (led_index > EXAMPLE_LED_NUMBERS - 1 - skip_pixels) {
//             led_direction = MOVE_FORWARD;
//             led_index = 0;
//         }
//     }
// }

// void ws2812_loop(uint64_t current_time) {
//     // Clear all LEDs
//     memset(led_strip_pixels, 0, sizeof(led_strip_pixels));

//     // Turn on the current LED
//     request_update_leds(led_index, rgb_value);
//     rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);

//     // Delay before moving to the next LED
//     vTaskDelay(pdMS_TO_TICKS(100));  // Adjust this value to change the speed of movement

//     // Move to the next LED position
//     if (led_direction == MOVE_FORWARD) {
//         led_index += (1 + skip_pixels);
//         if (led_index >= EXAMPLE_LED_NUMBERS) {
//             led_direction = MOVE_BACKWARD;
//             led_index = EXAMPLE_LED_NUMBERS - 1;
//         }
//     } else {
//         led_index -= (1 + skip_pixels);
//         if (led_index < 0) {
//             led_direction = MOVE_FORWARD;
//             led_index = 0;
//         }
//     }
// }
