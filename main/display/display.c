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
static i2c_device_t *sht31 = NULL;          // address 0x44 or 0x45

void display_setup(uint8_t scl_pin, uint8_t sda_pin) {
    esp_err_t ret = i2c_setup(scl_pin, sda_pin);

    ssd1306_setup(0x3C);
    ssd1306_print_str("Hello Bee", 0);

    bh1750 = i2c_device_create(I2C_NUM_0, 0x23);
    ret = i2c_write_byte(bh1750, 0x01);           // BH1750 POWER_ON = 0x01, POWER_DOWN = 0x00
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ERROR POWER_ON BH1750");
    }

    sht31 = i2c_device_create(I2C_NUM_0, 0x44);
    ret = i2c_write_command(sht31, 0x30, 0xA2);     // SOFT_RESET 0x30A2
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ERROR SOFT_RESET SHT31");
    }
}

void display_print_str(const char *str, uint8_t line) {
    ssd1306_print_str(str, line);
}


uint64_t interval_ref = 0;

//! bh1750
static esp_err_t bh1750_get_reading(uint64_t current_time) {
    if (current_time - interval_ref < 300000) return ESP_FAIL;
    interval_ref = current_time;

    char buff[32];
    uint8_t readings[2] = {0};
    uint8_t bh_mode[1] = { (uint8_t)BH1750_CONTINUOUS_4LX_RES };

    esp_err_t ret = i2c_write_read(bh1750, bh_mode, sizeof(bh_mode), readings, sizeof(readings));
    float bh1750_data = (readings[0] << 8 | readings[1]) / BH1750_READING_ACCURACY;
    snprintf(buff, sizeof(buff), "BH1750: %f", bh1750_data);
    ssd1306_print_str(buff, 2);

    return ret;
}

//! sht31
uint64_t sht31_wait_time = 30000;
uint64_t sht31_timeRef = 0;

static esp_err_t sht31_get_readings(uint64_t current_time) {
    if (current_time - sht31_timeRef < sht31_wait_time) return ESP_FAIL;
    sht31_timeRef = current_time;

    esp_err_t ret;
    char buff[32];
    uint8_t sht_readings[6] = {0};

    if (sht31_wait_time == 30000) {
        uint8_t sht_cmd[2] = { 0x24, 0x00 };
        ret = i2c_write_command(sht31, 0x24, 0x00);     // HIGHREP 0x2400
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ERROR READING SHT31");
            return ret;
        }
        sht31_wait_time = 300000;
    } else {
        ret = i2c_read(sht31, sht_readings, sizeof(sht_readings));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ERROR READING2 SHT31");
            return ret;
        }
    
        uint16_t temp_raw = (sht_readings[0] << 8) | sht_readings[1];
        uint16_t hum_raw = (sht_readings[3] << 8) | sht_readings[4];
        float temp = -45.0f + 175.0f * ((float)temp_raw / 65535.0f);
        float hum = 100.0f * ((float)hum_raw / 65535.0f);
    
        snprintf(buff, sizeof(buff), "Temp: %f", temp);
        ssd1306_print_str(buff, 3);
        snprintf(buff, sizeof(buff), "hum: %f", hum);
        ssd1306_print_str(buff, 4);
        sht31_wait_time = 30000;
    }

    return ret;
}

void i2c_sensor_readings(uint64_t current_time) {
    bh1750_get_reading(current_time);
    sht31_get_readings(current_time);
}
