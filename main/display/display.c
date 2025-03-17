#include "display.h"

#include "mod_ssd1306.h"
#include "driver/i2c.h"

#include "esp_log.h"
#include "esp_err.h"
#include "mod_i2c.h"

static const char *TAG = "I2C_DEVICE";

typedef enum {
    BH1750_CONTINUOUS_1LX       = 0x10,     // 1 lux resolution 120ms
    BH1750_CONTINUOUS_HALFLX    = 0x11,     // .5 lux resolution 120ms
    BH1750_CONTINUOUS_4LX_RES   = 0x13,     // 4 lux resolution 16ms
    BH1750_ONETIME_1LX          = 0x20,     // 1 lux resolution 120ms
    BH1750_ONETIME_HALFLX       = 0x21,     // .5 lux resolution 120ms
    BH1750_ONETIME_4LX          = 0x23,     // 4 lux resolution 16ms

} bh1750_mode_t;

#define BH1750_READING_ACCURACY    1.2

static i2c_device_t *bh1750 = NULL;

void display_setup(uint8_t scl_pin, uint8_t sda_pin, uint8_t address) {
    i2c_setup(scl_pin, sda_pin);

    ssd1306_setup(address);
    ssd1306_print_str("Hello Bee", 0);

    bh1750 = i2c_device_create(I2C_NUM_0, 0x23);

    // BH1750 POWER_ON = 0x01, POWER_DOWN = 0x00
    esp_err_t ret = i2c_write_byte_slow(bh1750, 0x01);
}

void display_print_str(const char *str, uint8_t line) {
    ssd1306_print_str(str, line);
}

void print_bh1750_readings() {
    uint8_t readings[2];
    esp_err_t ret = i2c_write_byte_slow(bh1750, BH1750_CONTINUOUS_4LX_RES);
    ret = i2c_read_bytes_slow(bh1750, readings, sizeof(readings));

    float bh1750_data = (readings[0] << 8 | readings[1]) / BH1750_READING_ACCURACY;
    ESP_LOGI(TAG, "bh1750 reading: %f\n", bh1750_data);
}