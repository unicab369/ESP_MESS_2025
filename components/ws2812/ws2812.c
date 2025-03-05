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
    led_pixels[index * 3] = rgb.green;          // Green
    led_pixels[index * 3 + 1] = rgb.red;        // Red
    led_pixels[index * 3 + 2] = rgb.blue;       // Blue
}

static void on_pulse_handler(uint8_t index, bool state) {
    ws2812_cyclePulse_t* obj = &ws2812_pulse_objs[index];
    RGB_t output = state ? obj->rgb : rgb_off;
    request_update_leds(obj->led_index, output);
}


static ws2812_cycleFade_t cycle_fades[OBJECT_COUNT];

void ws2812_load_fadeColor(ws2812_cycleFade_t ref, uint8_t index) {
    cycle_fades[index] = ref;
}

static void on_cycleFade_cb(uint16_t obj_index, int16_t fading_value) {
    RGB_t new_color;
    hsv_to_rgb(fading_value, 1.0f, 1.0f, &new_color);

    ws2812_cycleFade_t* ref = &cycle_fades[obj_index];
    RGB_t new_color2 = set_color_byChannels(fading_value, ref->active_channels);
    request_update_leds(ref->led_index, new_color2);
}

//! fill_sequence
step_sequence_config_t fill_sequence = {
    .current_value = 2,
    .direction = true,
    .is_bounced = true,
    .is_toggled = true,
    .is_uniformed = false,
    .increment = 1,
    .start_index = 2,
    .end_index = 5,
    .refresh_time_uS = 600000,
    .last_refresh_time = 0
};

static void fill_sequence_callback(uint8_t obj_index, step_sequence_config_t* conf) {
    RGB_t rgb_on = { .red = 150, .green = 0, .blue = 0 };
    RGB_t value = conf->is_toggled ? rgb_on : rgb_off;
    request_update_leds(conf->current_value, value);
}

//! step_sequence
step_sequence_config_t step_sequence = {
    .current_value = 1,
    .previous_value = -1,
    .direction = false,
    .is_bounced = true,
    .is_uniformed = true,
    .increment = 1,
    .start_index = 1,
    .end_index = 4,
    .refresh_time_uS = 200000,
    .last_refresh_time = 0
};

static void step_sequence_callback(uint8_t obj_index, step_sequence_config_t* conf) {
    RGB_t value = { .red = 0, .green = 0, .blue = 150 };

    // turn off the previous LED
    if (conf->previous_value >= 0) {
        request_update_leds(conf->previous_value, (RGB_t){0, 0, 0});
    }

    request_update_leds(conf->current_value, value);
}

//! hue animation

hue_animation_t hue_ani2 = {
    .hue = 0,
    .direction = true,
    .is_bounced = false,
    .start_index = 0,
    .end_index = LEDS_COUNT - 1,
    .value = 30,
    .speed = 1,
    .refresh_time_uS = 20,
    .last_refresh_time = 0
};

void hue_animation(uint64_t current_time) {
    if (current_time - hue_ani2.last_refresh_time < hue_ani2.refresh_time_uS) return;
    hue_ani2.last_refresh_time = current_time;

    for (int i = hue_ani2.start_index; i <= hue_ani2.end_index; i++) {
        RGB_t rgb_value;
        uint8_t offset = hue_ani2.direction ?  hue_ani2.end_index - i : i;

        hsv_to_rgb_ints((hue_ani2.hue + offset * 10) % 256, 255, 40, &rgb_value);
        request_update_leds(i, rgb_value);
    }

    // Increment hue for animation
    hue_ani2.hue = (hue_ani2.hue + hue_ani2.speed) % 256;

    if (hue_ani2.is_bounced) {
        // Switch direction when hue completes a full cycle
        if (hue_ani2.hue == 0) {
            hue_ani2.direction = ! hue_ani2.direction;  // Toggle direction
        }
    }
}


void ws2812_loop(uint64_t current_time) {
    //! handle hue animation
    // hue_animation(current_time);

    //! handle filling leds
    // cycle_indexes(current_time, 0, &fill_sequence, fill_sequence_callback);

    //! handle stepping led
    // cycle_indexes(current_time, 0, &step_sequence, step_sequence_callback);

    //! handle pulsing led
    // for (int i=0; i < OBJECT_COUNT; i++) {
    //     timer_pulse_obj_t* obj = &timer_objs[i];
    //     timer_pulse_handler(current_time, i, obj, on_pulse_handler);
    // }

    //! handle fading led
    for (int i=0; i < OBJECT_COUNT; i++) {
        cycle_fade(current_time, i, &cycle_fades[i].config, on_cycleFade_cb);
    }

    //! transmit the updated leds
    if (current_time - last_transmit_time < WS2812_TRANSMIT_FREQUENCY) return;
    last_transmit_time = current_time;
    rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
}



//! hue animation

// void ws2812_loop(uint64_t current_time) {
//     static float offset = 0;
//     static float angle_increment = 0.3;

//     for (int led = 0; led < LEDS_COUNT; led++) {
//         float angle = offset + (led * angle_increment);
//         const float color_off = (M_PI * 2) / 3;

//         // request_update_leds(led, )
//         led_pixels[led * 3] = sin(angle + color_off * 0) * 127 + 128;
//         led_pixels[led * 3 + 1] = sin(angle + color_off * 1) * 127 + 128;
//         led_pixels[led * 3 + 2] = sin(angle + color_off * 2) * 127 + 128;;
//     }

//     //Increase offset to shift pattern
//     offset += 0.02;
//     if (offset > 2 * M_PI) offset -= 2 * M_PI;

//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
//     vTaskDelay(pdMS_TO_TICKS(20));
// }


//! Trailing star start
// typedef struct {
//     RGB_t color;             // Color of the star
//     uint8_t length;          // Length of the star and tail
//     uint8_t speed;           // Speed of the star (in LEDs per frame)
//     uint16_t frame_delay;    // Delay between frames (in milliseconds)
//     bool direction;          // true = left-to-right, false = right-to-left
//     bool is_bounced;
// } StarConfig_t;


// // Configure the trailing star animation
// StarConfig_t config = {
//     .color = {255, 0, 0},   // Red color
//     .length = 3,            // Length of the star and tail
//     .speed = 1,             // Speed of the star
//     .frame_delay = 100,      // 50ms delay between frames
//     .direction = true,      // Left-to-right direction
//     .is_bounced = true
// };


// void ws2812_loop(uint64_t current_time) {
//     static int16_t star_position = 0; 

//     // Clear all LEDs
//     memset(led_pixels, 0, sizeof(led_pixels));

//     if (config.is_bounced) {
//         // Bouncing
//         if (config.direction) {
//             star_position += config.speed;

//             if (star_position >= LEDS_COUNT) {
//                 star_position = LEDS_COUNT - 1;         // Stay within bounds
//                 config.direction = false;               // Reverse direction
//             }
//         } else {
//             star_position -= config.speed;

//             if (star_position < -config.length) {
//                 star_position = -config.length + 1;     // Stay within bounds
//                 config.direction = true;                // Reverse direction
//             }
//         }
//     } else {
//         // Update star position
//         if (config.direction) {
//             star_position += config.speed;

//             if (star_position >= LEDS_COUNT) {
//                 star_position = -config.length;     // Reset to start (left side)
//             }
//         } else {
//             star_position -= config.speed;

//             if (star_position < -config.length) {
//                 star_position = LEDS_COUNT;         // Reset to end (right side)
//             }
//         }
//     }

//     // Draw star and tail
//     for (uint8_t i = 0; i < config.length; i++) {
//         int16_t pos = config.direction ? (star_position - i) : (star_position + i);

//         if (pos >= 0 && pos < LEDS_COUNT) {
//             uint8_t fade = 255 >> i; // Fade tail (brightness decreases by half each step)
            
//             request_update_leds(pos, (RGB_t){
//                 .red = (config.color.red * fade) >> 8,
//                 .green = (config.color.green * fade) >> 8,
//                 .blue = (config.color.blue * fade) >> 8
//             });
//         }
//     }

//     // // Draw wave 1 and its tail
//     // for (uint8_t i = 0; i < wave1->length; i++) {
//     //     int16_t pos = wave1->direction ? (wave1->position - i) : (wave1->position + i);
//     //     if (pos >= 0 && pos < NUM_LEDS) {
//     //         uint8_t fade = 255 >> i; // Fade tail (brightness decreases by half each step)
//     //         led_pixels[pos * 3] += (wave1->color.red * fade) >> 8;
//     //         led_pixels[pos * 3 + 1] += (wave1->color.green * fade) >> 8;
//     //         led_pixels[pos * 3 + 2] += (wave1->color.blue * fade) >> 8;
//     //     }
//     // }

//     // // Draw wave 2 and its tail
//     // for (uint8_t i = 0; i < wave2->length; i++) {
//     //     int16_t pos = wave2->direction ? (wave2->position - i) : (wave2->position + i);
//     //     if (pos >= 0 && pos < NUM_LEDS) {
//     //         uint8_t fade = 255 >> i; // Fade tail (brightness decreases by half each step)
//     //         led_pixels[pos * 3] += (wave2->color.red * fade) >> 8;
//     //         led_pixels[pos * 3 + 1] += (wave2->color.green * fade) >> 8;
//     //         led_pixels[pos * 3 + 2] += (wave2->color.blue * fade) >> 8;
//     //     }
//     // }


//     // Transmit the updated LED data
//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);

//     // Delay for the next frame
//     vTaskDelay(pdMS_TO_TICKS(config.frame_delay));
// }
//! Trailing star end


// void ws2812_loop(uint64_t current_time) {
//     request_update_leds(5, (RGB_t){ 255, 0, 0});
//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
//     vTaskDelay(pdMS_TO_TICKS(500));

//     request_update_leds(5, (RGB_t){ 0, 0, 0});
//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
//     vTaskDelay(pdMS_TO_TICKS(500));
// }
