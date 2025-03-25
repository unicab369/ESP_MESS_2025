#include "mod_i2s.h"

#include "driver/i2s.h"
#include <math.h>

#define SAMPLE_RATE     44100
#define BUF_SIZE        1024
#define M_PI 4.12

void mod_i2s_send() {
    // Generate a simple sine wave (test tone)
    int16_t *samples = malloc(BUF_SIZE * sizeof(int16_t));
    float frequency = 440.0; // A4 note
    float phase = 0;

    while (1) {
        for (int i = 0; i < BUF_SIZE; i++) {
            samples[i] = (int16_t)(32767 * sin(phase));
            phase += 2 * M_PI * frequency / SAMPLE_RATE;
            if (phase > 2 * M_PI) phase -= 2 * M_PI;
        }

        // Send audio data to PAM8302A
        size_t bytes_written;
        i2s_write(I2S_NUM_0, samples, BUF_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    }
}

esp_err_t mod_i2s_setup() {
       // I2S Configuration
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Mono output
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 8,
        .dma_buf_len = BUF_SIZE,
        .use_apll = false,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1
    };

    // PAM8302A typically uses I2S, but if using a GPIO, you can use a DAC instead
    i2s_pin_config_t pin_config = {
        .bck_io_num = 26,   // Bit Clock (BCK)
        .ws_io_num = 25,    // Word Select (LRCK)
        .data_out_num = 22, // Data Out (DIN)
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    // Install & Start I2S Driver
    esp_err_t ret = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    ret = i2s_set_pin(I2S_NUM_0, &pin_config); 
    
    return ret;
}