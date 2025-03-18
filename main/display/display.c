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
static i2c_device_t *ap3216 = NULL;
static i2c_device_t *apds9960 = NULL;

#define APDS9960_ADDR 0x39        // I2C address (7-bit)
#define I2C_MASTER_NUM I2C_NUM_0  // I2C port
#define I2C_TIMEOUT_MS 1000

// Register addresses
#define ENABLE 0x80
#define CONFIG2 0x90   // Configuration

void display_setup(uint8_t scl_pin, uint8_t sda_pin) {
    esp_err_t ret = i2c_setup(scl_pin, sda_pin);

    ssd1306_setup(0x3C);
    ssd1306_print_str("Hello Bee", 0);

    //! bh1750
    bh1750 = i2c_device_create(I2C_NUM_0, 0x23);
    ret = i2c_write_byte(bh1750, 0x01);           // BH1750 POWER_ON = 0x01, POWER_DOWN = 0x00
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR POWER_ON BH1750");

    //! sht31
    sht31 = i2c_device_create(I2C_NUM_0, 0x44);
    ret = i2c_write_command(sht31, 0x30, 0xA2);     // SOFT_RESET 0x30A2
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR SOFT_RESET SHT31");

    //! AP3216
    ap3216 = i2c_device_create(I2C_NUM_0, 0x1E);
    ret = i2c_write_command(ap3216, 0x00, 0x03);    // RESET
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR SOFT_RESET AP3216");

    //! APDS9960
    uint8_t config[] = {
        ENABLE, 0x0F,  // Power ON, Proximity enable, ALS enable, Wait enable
        CONFIG2, 0x01, // Proximity gain control (4x)
        0x8F, 0x20,    // Proximity pulse count (8 pulses)
        0x8E, 0x87     // Proximity pulse length (16us)
    };

    apds9960 = i2c_device_create(I2C_NUM_0, 0x39);
    ret = i2c_write(apds9960, config, sizeof(config));
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR SOFT_RESET APDS9960");
}

void display_print_str(const char *str, uint8_t line) {
    ssd1306_print_str(str, line);
}

//! bh1750
static esp_err_t bh1750_get_reading() {
    char buff[32];
    esp_err_t ret;

    uint8_t readings[2] = {0};
    uint8_t bh_mode[1] = { (uint8_t)BH1750_CONTINUOUS_4LX_RES };

    ret = i2c_write_read(bh1750, bh_mode, sizeof(bh_mode), readings, sizeof(readings));
    float bh1750_data = (readings[0] << 8 | readings[1]) / BH1750_READING_ACCURACY;
    snprintf(buff, sizeof(buff), "BH1750: %f", bh1750_data);
    ssd1306_print_str(buff, 2);
    return ret;
}

//! AP3216
static esp_err_t ap3216_get_reading() {
    uint8_t valHi; uint8_t valLo;
    esp_err_t ret = i2c_write_read_byte(ap3216, 0x0F, &valHi);
    ret = i2c_write_read_byte(ap3216, 0x0E, &valLo);

    int hiByte = valHi & 0b00111111; // PS HIGH_REG 6 bits
    int loBite = valLo & 0b00001111; // PS LOW_REG 4 bits
    uint16_t ps = (hiByte << 4) + loBite;

    ret = i2c_write_read_byte(ap3216, 0x0D, &valHi);
    ret = i2c_write_read_byte(ap3216, 0x0C, &valLo);
	uint16_t als = (valHi << 8) + valLo;

    char buff[32];
    snprintf(buff, sizeof(buff), "prox: %u. als: %u", ps, als);
    ssd1306_print_str(buff, 5);
    return ret;
}

//! ADPS9960
static esp_err_t apds9960_get_reading() {
    esp_err_t ret;

    char buff[64];
    uint8_t prox;
    ret = i2c_write_read_byte(apds9960, 0x9C, &prox);      // PROX DATA 0x9c

    // Clear Light LowByte
    uint8_t raw[8] = { 0 };
    ret = i2c_write_read_command(apds9960, 0x94, raw, sizeof(raw));

    uint16_t clear  = (raw[1] << 8) | raw[0];
    uint16_t red    = (raw[3] << 8) | raw[2];
    uint16_t green  = (raw[5] << 8) | raw[4];
    uint16_t blue   = (raw[7] << 8) | raw[6];

    // Method 1: Direct clear channel conversion
    // 0.0576 * clear;

    // Method 2: RGB coefficients (CIE 1931)
    // (0.2126 * red) + (0.7152 * green) + (0.0722 * blue);
    snprintf(buff, sizeof(buff), "prox: %u, w: %u, r: %u, g: %u, b: %u", prox, clear, red, green, blue);
    ssd1306_print_str(buff, 6);

    return ret;
}

//! sht31
uint64_t sht31_wait_time = 30000;
uint64_t sht31_timeRef = 0;
uint64_t interval_ref = 0;

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
    sht31_get_readings(current_time);

    if (current_time - interval_ref < 300000) return;
    interval_ref = current_time;

    bh1750_get_reading();
    ap3216_get_reading();
    apds9960_get_reading();
}
