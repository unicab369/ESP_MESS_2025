#include "ws2812.h"
#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"

#define RMT_LED_STRIP_GPIO_NUM      12
#define EXAMPLE_LED_NUMBERS         7

#define EXAMPLE_FRAME_DURATION_MS   20
#define EXAMPLE_ANGLE_INC_FRAME     0.02
#define EXAMPLE_ANGLE_INC_LED       0.3

static const char *TAG = "example";

static uint8_t led_strip_pixels[EXAMPLE_LED_NUMBERS * 3];


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


#define WS2812_TRANSMIT_FREQUENCY 50000   // every 50ms

uint64_t last_transmit_time;

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

void ws2812_load_obj(timer_pulse_config_t config, timer_pulse_obj_t* object) {
    last_transmit_time = esp_timer_get_time();

    timer_pulse_setup(config, object);
    timer_pulse_reset(esp_timer_get_time(), object);
}

static void request_clear_leds() {
    memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
}

static void request_update_leds(uint16_t index, ws2812_rgb_t rgb) {
    led_strip_pixels[index * 3 + 0] = rgb.green;        // Green
    led_strip_pixels[index * 3 + 1] = rgb.red;          // Red
    led_strip_pixels[index * 3 + 2] = rgb.blue;         // Blue
}

void ws2812_toggle(bool state) {
    if (state) {
        ws2812_rgb_t rgb_value = { .red = 200, .green = 0, .blue = 0 };
        request_update_leds(4, rgb_value);
    } else {
        request_clear_leds();
    }
}

void ws2812_toggle2(bool state) {
    if (state) {
        ws2812_rgb_t rgb_value = { .red = 0, .green = 200, .blue = 0 };
        request_update_leds(3, rgb_value);
    } else {
        request_clear_leds();
    }
}

void ws2812_loop(uint64_t current_time, timer_pulse_obj_t* objects, size_t len) {
    timer_pulse_handler(current_time, objects, len);

    if (current_time - last_transmit_time <= WS2812_TRANSMIT_FREQUENCY) return;
    last_transmit_time = current_time;
    rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);
}

int led_index = 0;

void ws2812_loop2(uint64_t current_time) {
    // Clear all LEDs
    memset(led_strip_pixels, 0, sizeof(led_strip_pixels));

    ws2812_rgb_t rgb_value = { .red = 200, .green = 0, .blue = 0 };
    request_update_leds(led_index, rgb_value);

    // Delay before moving to the next LED
    vTaskDelay(pdMS_TO_TICKS(100));  // Adjust this value to change the speed of movement

    // Move to the next LED
    led_index = (led_index + 1) % EXAMPLE_LED_NUMBERS;
}

float offset = 0;

void ws2812_run1(uint64_t current_time) {
    for (int led = 0; led < EXAMPLE_LED_NUMBERS; led++) {
        // Build RGB pixels. Each color is an offset sine, which gives a
        // hue-like effect.
        float angle = offset + (led * EXAMPLE_ANGLE_INC_LED);
        const float color_off = (M_PI * 2) / 3;
        led_strip_pixels[led * 3 + 0] = sin(angle + color_off * 0) * 127 + 128;
        led_strip_pixels[led * 3 + 1] = sin(angle + color_off * 1) * 127 + 128;
        led_strip_pixels[led * 3 + 2] = sin(angle + color_off * 2) * 117 + 128;;
    }
    // Flush RGB values to LEDs
    rmt_transmit(led_chan, simple_encoder, led_strip_pixels, sizeof(led_strip_pixels), &tx_config);
    rmt_tx_wait_all_done(led_chan, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(EXAMPLE_FRAME_DURATION_MS));
    //Increase offset to shift pattern
    offset += EXAMPLE_ANGLE_INC_FRAME;
    if (offset > 2 * M_PI) {
        offset -= 2 * M_PI;
    }
}
