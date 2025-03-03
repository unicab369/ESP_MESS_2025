#include "ws2812.h"
#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"


#define RMT_LED_STRIP_GPIO_NUM      12
#define LEDS_COUNT         7

#define EXAMPLE_FRAME_DURATION_MS   20
#define EXAMPLE_ANGLE_INC_FRAME     0.02
#define EXAMPLE_ANGLE_INC_LED       0.3

static const char *TAG = "example";

static uint8_t led_pixels[LEDS_COUNT * 3];


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

#define OBJECT_COUNT 2
static timer_pulse_obj_t timer_objs[OBJECT_COUNT];
static ws2812_cyclePulse_t ws2812_pulse_objs[OBJECT_COUNT];

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

void ws2812_load_pulse(ws2812_cyclePulse_t object) {
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

static void request_update_leds(uint16_t index, RGB_t rgb) {
    led_pixels[index * 3 + 0] = rgb.green;        // Green
    led_pixels[index * 3 + 1] = rgb.red;          // Red
    led_pixels[index * 3 + 2] = rgb.blue;         // Blue
}

static void on_pulse_handler(uint8_t index, bool state) {
    ws2812_cyclePulse_t* obj = &ws2812_pulse_objs[index];
    RGB_t output = state ? obj->rgb : rgb_off;
    request_update_leds(obj->led_index, output);
}


//! Cycle Index
sequence_config_t cycleIndex = {
    .current_value = 0,
    .is_switched = true,
    .increment = 1,
    .max_value = 7,
    .refresh_time_uS = 60000,
    .last_refresh_time = 0
};

static void on_cycleIndex_cb(int16_t index, bool is_switched) {
    RGB_t rgb_on = { .red = 0, .green = 0, .blue = 150 };
    RGB_t value = is_switched ? rgb_on : rgb_off;
    request_update_leds(index, value);
}

static ws2812_cycleFade_t cycle_fades[OBJECT_COUNT];

static void on_cycleFade_cb(uint16_t obj_index, int16_t fading_value) {
    RGB_t new_color;
    hsv_to_rgb(fading_value, 1.0f, 1.0f, &new_color);

    ws2812_cycleFade_t* ref = &cycle_fades[obj_index];
    RGB_t new_color2 = set_color_byChannels(fading_value, ref->active_channels);
    request_update_leds(ref->led_index, new_color2);
}


void ws2812_load_fadeColor(ws2812_cycleFade_t ref, uint8_t index) {
    cycle_fades[index] = ref;
}

void ws2812_loop(uint64_t current_time) {
    //! handle pulses
    for (int i=0; i < OBJECT_COUNT; i++) {
        timer_pulse_obj_t* obj = &timer_objs[i];
        timer_pulse_handler(current_time, i, obj, on_pulse_handler);
    }

    //! handle fading leds
    for (int i=0; i < OBJECT_COUNT; i++) {
        cycleFade_check(current_time, i, &cycle_fades[i].config, on_cycleFade_cb);
    }

    //! handle moving leds
    cycleIndex_check(current_time, &cycleIndex, on_cycleIndex_cb);

    //! transmit the updated leds
    if (current_time - last_transmit_time < WS2812_TRANSMIT_FREQUENCY) return;
    last_transmit_time = current_time;
    rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
}


float offset = 0;

void ws2812_run1(uint64_t current_time) {
    for (int led = 0; led < LEDS_COUNT; led++) {
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
RGB_t rgb_value = { .red = 200, .green = 0, .blue = 0 };
int led_index = 0;

// void ws2812_loop(uint64_t current_time) {
//     // Clear all LEDs
//     memset(led_pixels, 0, sizeof(led_pixels));

//     // Turn on the current LED
//     request_update_leds(led_index, rgb_value);
//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);

//     // Delay before moving to the next LED
//     vTaskDelay(pdMS_TO_TICKS(100));  // Adjust this value to change the speed of movement

//     // Move to the next LED position
//     if (led_direction == MOVE_FORWARD) {
//         led_index += (1 + skip_pixels);
//         if (led_index >= LEDS_COUNT) {
//             led_direction = MOVE_BACKWARD;
//             led_index = LEDS_COUNT - 1;
//         }
//     } else {
//         led_index -= (1 + skip_pixels);
//         if (led_index < 0) {
//             led_direction = MOVE_FORWARD;
//             led_index = 0;
//         }
//     }
// }

// void ws2812_loop(uint64_t current_time) {
//     request_update_leds(5, (RGB_t){ 255, 0, 0});
//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
//     vTaskDelay(pdMS_TO_TICKS(500));

//     request_update_leds(5, (RGB_t){ 0, 0, 0});
//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
//     vTaskDelay(pdMS_TO_TICKS(500));
// }


#define SKIP_STEPS 1

// void ws2812_loop(uint64_t current_time) {
//     static int current_led = 0;
//     static int previous_led = 0;

//     // Turn off the previous LED
//     if (previous_led >= 0) {
//         request_update_leds(previous_led, (RGB_t){0, 0, 0});
//     }

//     // Turn on the current LED
//     request_update_leds(current_led, (RGB_t){255, 0, 0});
    
//     // Move to the next LED
//     previous_led = current_led;
//     current_led = (current_led + SKIP_STEPS) % LEDS_COUNT;

//     // If we've wrapped around, adjust the current_led to avoid skipping the first LED
//     if (current_led < SKIP_STEPS) {
//         current_led = 0;
//     }

//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);

//     printf("current = %d, previous = %d\n", current_led, previous_led);
//     vTaskDelay(pdMS_TO_TICKS(1000));
// }

// void ws2812_loop(uint64_t current_time) {
//     static int current_led = LEDS_COUNT - 1;  // Start from the last LED
//     static int previous_led = LEDS_COUNT - 1;

//     // Turn off the previous LED
//     if (previous_led >= 0) {
//         request_update_leds(previous_led, (RGB_t){0, 0, 0});
//     }

//     // Turn on the current LED
//     request_update_leds(current_led, (RGB_t){255, 0, 0});
    
//     // Move to the previous LED
//     previous_led = current_led;
//     current_led = (current_led - SKIP_STEPS + LEDS_COUNT) % LEDS_COUNT;

//     // If we've wrapped around, adjust the current_led to avoid skipping the last LED
//     if (current_led > LEDS_COUNT - SKIP_STEPS) {
//         current_led = LEDS_COUNT - 1;
//     }

//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);

//     printf("current = %d, previous = %d\n", current_led, previous_led);
//     vTaskDelay(pdMS_TO_TICKS(1000));
// }



// static bool reverse = false;

// void ws2812_loop(uint64_t current_time) {
//     static int current_led = 0;
//     static int previous_led = 0;
//     static int skip_steps2 = 2;

//     // Turn off the previous LED
//     if (previous_led >= 0) {
//         request_update_leds(previous_led, (RGB_t){0, 0, 0});
//     }

//     // Turn on the current LED
//     request_update_leds(current_led, (RGB_t){255, 0, 0});
    
//     // Move to the next LED
//     previous_led = current_led;

//     if (!reverse) {
//         // Forward movement
//         current_led = (current_led + skip_steps2) % LEDS_COUNT;
//         if (current_led < skip_steps2) {
//             current_led = 0;
//         }
//     } else {
//         // Backward movement
//         current_led = (current_led - skip_steps2 + LEDS_COUNT) % LEDS_COUNT;
//         if (current_led > LEDS_COUNT - skip_steps2) {
//             current_led = LEDS_COUNT - 1;
//         }
//     }

//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);

//     printf("current = %d, previous = %d, direction = %s\n", current_led, previous_led, reverse ? "backward" : "forward");
//     vTaskDelay(pdMS_TO_TICKS(800));
// }


// void ws2812_loop(uint64_t current_time) {
//     static int current_led = 0;
//     static int previous_led = 0;
//     static bool reverse = true;

//     // Turn off the previous LED
//     if (previous_led >= 0) {
//         request_update_leds(previous_led, (RGB_t){0, 0, 0});
//     }

//     // Turn on the current LED
//     request_update_leds(current_led, (RGB_t){255, 0, 0});
//     previous_led = current_led;

//     if (!reverse) {
//         current_led += SKIP_STEPS;

//         // wrapped around
//         if (current_led >= LEDS_COUNT) {
//             reverse = true;
//             current_led = LEDS_COUNT - 1;
//         }
//     } else {
//         current_led -= SKIP_STEPS;

//         // wrapped around
//         if (current_led < 0) {
//             reverse = false;
//             current_led = 0;
//         }
//     }

//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);

//     printf("current = %d, previous = %d, direction = %s\n", current_led, previous_led, reverse ? "reverse" : "forward");

//     vTaskDelay(pdMS_TO_TICKS(1000));
// }