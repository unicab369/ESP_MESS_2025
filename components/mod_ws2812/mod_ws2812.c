#include "mod_ws2812.h"
#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"

#define LEDS_COUNT         9


static const char *TAG = "MOD_WS2812";
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

void ws2812_setup(uint8_t gpio_pin) {
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = gpio_pin,
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

static void request_clear_allLeds() {
    memset(led_pixels, 0, sizeof(led_pixels));
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

static void fade_sequence_callback(uint8_t obj_index, step_sequence_config_t* conf) {
    RGB_t new_color;
    int16_t curentValue = conf->current_value;
    hsv_to_rgb(curentValue, 1.0f, 1.0f, &new_color);

    ws2812_cycleFade_t* ref = &cycle_fades[obj_index];
    RGB_t new_color2 = make_color_byChannels(curentValue, ref->active_channels);
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

static void fill_sequence_callback(uint8_t obj_index, step_sequence_config_t* conf) __attribute__((unused));
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

static void step_sequence_callback(uint8_t obj_index, step_sequence_config_t* conf) __attribute__((unused));
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
    .refresh_time_uS = 100,
    .last_refresh_time = 0
};

void hue_animation(uint64_t current_time) {
    if (current_time - hue_ani2.last_refresh_time < hue_ani2.refresh_time_uS) return;
    hue_ani2.last_refresh_time = current_time;

    for (int i = hue_ani2.start_index; i <= hue_ani2.end_index; i++) {
        RGB_t rgb_value;
        uint8_t offset = hue_ani2.direction ?  hue_ani2.end_index - i : i;

        hsv_to_rgb_ints((hue_ani2.hue + offset * 10) % 256, 255, 10, &rgb_value);
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

// update this code to make the wave move by adjusting the phase instead of is_moving

#define SEQUENCE_LENGTH 5

typedef struct {
    uint16_t current_index;
    RGB_t neigative_color;
    int offset;
    int total_period;
    float frequency;
    bool is_bounced;
    int8_t direction;
    uint8_t length;
    uint8_t gap;
    uint64_t refresh_time_uS;
    uint64_t last_refresh_time;
    float phase;
} sequenced_wave_t;

sequenced_wave_t sequence1 = {
    .current_index = 0,
    .neigative_color = { 0, 0, 0 },
    .offset = 0,
    .total_period = SEQUENCE_LENGTH + LEDS_COUNT,
    .frequency = (2 * M_PI) / SEQUENCE_LENGTH,
    .is_bounced = true,
    .direction = 1,
    .length = LEDS_COUNT,
    .gap = 3,
    .refresh_time_uS = 800000,
    .last_refresh_time = 0,
    .phase = 0,
};

//! moving wave: repeated sequence
void moving_wave1(uint64_t current_time) {
    if (current_time - sequence1.last_refresh_time < sequence1.refresh_time_uS) return;
    sequence1.last_refresh_time = current_time;

    sequence1.phase += -0.1f * sequence1.direction;
    if (sequence1.phase >= 2 * M_PI) sequence1.phase -= 2 * M_PI;
    
    uint8_t cycle_length = sequence1.length;

    for (int i = 0; i < LEDS_COUNT; i++) {
        int position = i % (cycle_length + sequence1.gap);
        request_update_leds(i, sequence1.neigative_color);

        if (position < cycle_length) {
            float sin_offset = 1.0f + sinf((position / (float)cycle_length) * 2 * M_PI + sequence1.phase);
            uint8_t brightness = (uint8_t)(127.5f * sin_offset);
            RGB_t color = { brightness, 0, 0 };
            request_update_leds(i, color);
        }
    }
}

//! moving wave: single sequence
void moving_wave2(uint64_t current_time) {
    if (current_time - sequence1.last_refresh_time < sequence1.refresh_time_uS) return;
    sequence1.last_refresh_time = current_time;
    
    if (sequence1.is_bounced) {
        sequence1.offset += sequence1.direction;
        
        // printf("offset = %d\n", sequence1.offset);    
        // check for dirrection change
        if ((sequence1.offset >= sequence1.total_period + SEQUENCE_LENGTH)
            || (sequence1.offset <= SEQUENCE_LENGTH && sequence1.direction != 1)) {
                sequence1.direction = sequence1.direction * -1;
        } 
    } else {
        int offsetAdjustment = (sequence1.direction == 1) ? sequence1.total_period - 1 : 1;
        sequence1.offset = (offsetAdjustment + sequence1.offset) % sequence1.total_period;
    }

    for (int i = 0; i < LEDS_COUNT; i++) {
        int position = (i + sequence1.offset) % sequence1.total_period;        
        request_update_leds(i, sequence1.neigative_color);

        // printf("%d ", position);
        if (position < SEQUENCE_LENGTH) {
            float sin_offset = 1.0f + sinf(position * sequence1.frequency);
            uint8_t brightness = (uint8_t)(127.5f * sin_offset);
            RGB_t color = { brightness, 0, 0 };
            request_update_leds(i, color);
        }
    }
    // printf("\nDONE\n");
}

//! moving wave: expanding sequence
void moving_wave3(uint64_t current_time) {
    int8_t center_led = LEDS_COUNT / 2;

    if (current_time - sequence1.last_refresh_time < sequence1.refresh_time_uS) return;
    sequence1.last_refresh_time = current_time;

    static int8_t expansion = 0;
    expansion += sequence1.direction;

    // Change direction if limits are reached
    if ((expansion >= center_led && sequence1.direction != -1)
        || (expansion < 0 && sequence1.direction != 1)) {
            sequence1.direction *= -1;
    }
    
    // printf("expansion = %d\n", expansion);
    request_clear_allLeds();

    RGB_t value = (RGB_t){255, 0, 0};
    if (expansion >= 0) {
        request_update_leds(center_led, value);  // Center LED on only when not fully contracted
    }
    for (int i = 1; i <= expansion; i++) {
        uint16_t upperBound = center_led + i;
        uint16_t lowerBound = center_led - i;

        if (upperBound < LEDS_COUNT) {
            request_update_leds(upperBound, value);  // Right side
        }
        if (lowerBound >= 0) {
            request_update_leds(lowerBound, value);  // Left side
        }
    }
}


RGB_t gradient_array[LEDS_COUNT] = {
    {255, 0, 0},
    {255, 127, 0},
    {255, 255, 0},
    {0, 255, 0},
    {0, 0, 255}
};

static uint32_t last_update = 0;

//! moving wave: fixed gradient array;
void moving_wave4(uint64_t current_time) {
    if (current_time - last_update < 100000) return;
    last_update = current_time;

    memset(led_pixels, 0, sizeof(led_pixels));
    sequence1.offset = (sequence1.offset - sequence1.direction + LEDS_COUNT) % LEDS_COUNT;

    for (int i = 0; i < LEDS_COUNT; i++) {
        int position = (i + sequence1.offset) % LEDS_COUNT;
        RGB_t color = gradient_array[position];
        request_update_leds(i, color);
    }
}

typedef struct {
    int position;
    int value;          // brightness
    bool increasing;
} Star;

typedef struct {
    int start_index;
    int end_index;
    uint8_t twinkle_chance;
    uint8_t max_brightness;
    uint8_t fade_rate;
    uint64_t refresh_time_uS;
    uint64_t last_refresh_time;
} sequenced_star_t;

sequenced_star_t seq_star = {
    .start_index = 2,
    .end_index = 7,
    .twinkle_chance = 10,
    .max_brightness = 200,
    .fade_rate = 3,
    .refresh_time_uS = 50000,
    .last_refresh_time = 0
};

#define MAX_STARS 5
static Star stars[MAX_STARS] = {0};

void moving_wave5(uint64_t current_time) {
    memset(led_pixels, 0, sizeof(led_pixels));

    for (int i = 0; i < MAX_STARS; i++) {
        if (stars[i].value > 0) {
            // Update existing star
            if (stars[i].increasing) {
                stars[i].value += seq_star.fade_rate;

                if (stars[i].value >= seq_star.max_brightness) {
                    stars[i].value = seq_star.max_brightness;
                    stars[i].increasing = false;
                }
            } else {
                stars[i].value -= seq_star.fade_rate;
                if (stars[i].value <= 0) stars[i].value = 0;
            }

            RGB_t new_color;
            fill_color_byValue(&new_color, stars[i].value);
            request_update_leds(seq_star.start_index + stars[i].position, new_color);

        } else if (rand() % seq_star.twinkle_chance == 0) {
            // Create new star
            int effect_length = seq_star.end_index - seq_star.start_index + 1;
            
            stars[i].position = rand() % effect_length;
            stars[i].value = seq_star.fade_rate;
            stars[i].increasing = true;
        }
    }
}

void ws2812_loop(uint64_t current_time) {
    // moving_wave1(current_time);

    //! handle hue animation
    hue_animation(current_time);

    //! handle filling leds
    // cycle_values(current_time, 0, &fill_sequence, fill_sequence_callback);

    //! handle stepping led
    // cycle_values(current_time, 0, &step_sequence, step_sequence_callback);

    //! handle pulsing led
    // for (int i=0; i < OBJECT_COUNT; i++) {
    //     timer_pulse_obj_t* obj = &timer_objs[i];
    //     timer_pulse_handler(current_time, i, obj, on_pulse_handler);
    // }

    //! handle fading led
    // for (int i=0; i < OBJECT_COUNT; i++) {
    //     cycle_values(current_time, i, &cycle_fades[i].config, fade_sequence_callback);
    // }

    //! transmit the updated leds
    if (current_time - last_transmit_time < WS2812_TRANSMIT_FREQUENCY) return;
    last_transmit_time = current_time;
    rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
}


// // Define color points
// #define MAX_COLOR_POINTS 10
// RGB_t color_points[MAX_COLOR_POINTS] = {
//     {255, 0, 0},    // Red
//     {0, 255, 0},    // Green
//     {0, 0, 255},    // Blue
//     {255, 255, 0},  // Yellow
//     // {0, 255, 255}   // Cyan
// };
// const int num_color_points = 4; // Change this to use fewer or more color points

// #define GRADIENT_LENGTH (LEDS_COUNT * num_color_points)
// #define FIXED_POINT_SCALE 256 // 8-bit fractional part

// // Linear interpolation function using fixed-point arithmetic
// static uint8_t lerp(uint8_t a, uint8_t b, int t) {
//     return a + (((b - a) * t) >> 8);
// }

// void moving_wave4(uint64_t current_time) {
//     static int offset = 0;

//     // Clear all LEDs
//     memset(led_pixels, 0, sizeof(led_pixels));
//     // Move the gradient
//     offset = (offset + 1) % GRADIENT_LENGTH;

//     // Create gradient effect
//     for (int i = 0; i < LEDS_COUNT; i++) {
//         int position = (i + offset) % GRADIENT_LENGTH;
//         int section = position / (GRADIENT_LENGTH / num_color_points);
//         int t = (position % (GRADIENT_LENGTH / num_color_points)) * num_color_points * FIXED_POINT_SCALE / GRADIENT_LENGTH;

//         RGB_t start_color = color_points[section];
//         RGB_t end_color = color_points[(section + 1) % num_color_points];

//         uint8_t r = lerp(start_color.red, end_color.red, t);
//         uint8_t g = lerp(start_color.green, end_color.green, t);
//         uint8_t b = lerp(start_color.blue, end_color.blue, t);

//         request_update_leds(i, (RGB_t){r, g, b});
//     }

//     // Delay
//     vTaskDelay(pdMS_TO_TICKS(200));
// }



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



//! Example
// update this code to create a gradient light

// void moving_wave4(uint64_t current_time) {
//     request_update_leds(5, (RGB_t){ 255, 0, 0});
//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
//     vTaskDelay(pdMS_TO_TICKS(500));

//     request_update_leds(5, (RGB_t){ 0, 0, 0});
//     rmt_transmit(led_chan, simple_encoder, led_pixels, sizeof(led_pixels), &tx_config);
//     vTaskDelay(pdMS_TO_TICKS(500));
// }
