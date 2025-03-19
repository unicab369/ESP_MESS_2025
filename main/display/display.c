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


// Register addresses
#define ENABLE 0x80
#define CONFIG2 0x90   // Configuration

static i2c_device_t *bh1750 = NULL;
static i2c_device_t *sht31 = NULL;          // address 0x44 or 0x45
static i2c_device_t *ap3216 = NULL;
static i2c_device_t *apds9960 = NULL;
static i2c_device_t *max4400 = NULL;
static i2c_device_t *vl53lox = NULL;
static i2c_device_t *mpu6050 = NULL;
static i2c_device_t *ina219 = NULL;
static i2c_device_t *ds3231 = NULL;

// Convert BCD to decimal
static uint8_t bcd_to_decimal(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

// Convert decimal to BCD
static uint8_t decimal_to_bcd(uint8_t decimal) {
    return ((decimal / 10) << 4) | (decimal % 10);
}

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hr;
    uint8_t day;        // 1: Monday to 7: Sunday
    uint8_t month;
    uint16_t year;
    uint8_t date;
} ds3231_dateTime_t;

static esp_err_t ds3231_update_time(ds3231_dateTime_t datetime) {
    uint8_t time[3] = { datetime.sec, datetime.min, datetime.hr };
    return i2c_write_command_data(ds3231, 0x00, time, sizeof(time));
}

static esp_err_t ds3231_update_date(ds3231_dateTime_t datetime) {
    esp_err_t ret = i2c_write_command(ds3231, 0x04, decimal_to_bcd(datetime.date));
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR WRITE_DATE DS3231");

    ret = i2c_write_command(ds3231, 0x05, decimal_to_bcd(datetime.month));
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR WRITE_MONTH DS3231");

    ret = i2c_write_command(ds3231, 0x06, decimal_to_bcd(datetime.year-2000));
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR WRITE_YEAR DS3231");
    return ret;
}


void display_setup(uint8_t scl_pin, uint8_t sda_pin) {
    esp_err_t ret = i2c_setup(scl_pin, sda_pin);

    ssd1306_setup(0x3C);
    ssd1306_print_str("Hello Bee", 0);

    //! bh1750
    bh1750 = i2c_device_create(I2C_NUM_0, 0x23);
    ret = i2c_write_byte(bh1750, 0x01);           // BH1750 POWER_ON = 0x01, POWER_DOWN = 0x00
    if (ret != ESP_OK) ESP_LOGE(TAG, "BH1750 ERROR: POWER_ON");

    //! sht31
    sht31 = i2c_device_create(I2C_NUM_0, 0x44);
    ret = i2c_write_command(sht31, 0x30, 0xA2);     // SOFT_RESET 0x30A2
    if (ret != ESP_OK) ESP_LOGE(TAG, "SHT31 ERROR: SOFT_RESET");

    //! AP3216
    ap3216 = i2c_device_create(I2C_NUM_0, 0x1E);
    ret = i2c_write_command(ap3216, 0x00, 0x03);    // RESET
    if (ret != ESP_OK) ESP_LOGE(TAG, "AP3216 ERROR: SOFT_RESET");

    //! APDS9960
    uint8_t config[] = {
        ENABLE, 0x0F,  // Power ON, Proximity enable, ALS enable, Wait enable
        CONFIG2, 0x01, // Proximity gain control (4x)
        0x8F, 0x20,    // Proximity pulse count (8 pulses)
        0x8E, 0x87     // Proximity pulse length (16us)
    };

    apds9960 = i2c_device_create(I2C_NUM_0, 0x39);
    ret = i2c_write(apds9960, config, sizeof(config));
    if (ret != ESP_OK) ESP_LOGE(TAG, "APDS9960 ERROR: SOFT_RESET");

    //! MAX44009
    max4400 = i2c_device_create(I2C_NUM_0, 0x4A);       // AO to ground for ADDR 0x4B
    ret = i2c_write_command(max4400, 0x02, 0x80);       // 0x02 Config_Reg, 0x80 Config_value
    if (ret != ESP_OK) ESP_LOGE(TAG, "MAX44009 ERROR: SOFT_RESET");

    //! VL53LOX
    vl53lox = i2c_device_create(I2C_NUM_0, 0x29);

    uint8_t model_id;
    ret = i2c_write_read_byte(vl53lox, 0xC0, &model_id);
    if (ret != ESP_OK) ESP_LOGE(TAG, "VL53LOX ERROR: GET_MODEL");

    ret = i2c_write_command(vl53lox, 0x00, 0x01);       // write command to START_RANGING REG
    if (ret != ESP_OK) ESP_LOGE(TAG, "VL53LOX ERROR: SOFT_RESET");

    //! MPU6050
    mpu6050 = i2c_device_create(I2C_NUM_0, 0x69);       // ADO HI 0x69, unconnect 0x68
    ret = i2c_write_command(mpu6050, 0x6B, 0x00);       // WAKEUP REG
    if (ret != ESP_OK) ESP_LOGE(TAG, "MPU6050 ERROR: WAKEUP");

    //! INA219
    ina219 = i2c_device_create(I2C_NUM_0, 0x40);        // adjust A0 and A1 for 0x41, 0x44, 0x45

    uint8_t ina_config[2] = { 0x39, 0x9F };
    ret = i2c_write_command_data(ina219, 0x00, ina_config, sizeof(ina_config));
    if (ret != ESP_OK) ESP_LOGE(TAG, "INA219 ERROR: SOFT_RESET");

    //! DS3231
    ds3231 = i2c_device_create(I2C_NUM_0, 0x68);
    ds3231_dateTime_t new_time = {
        .sec = 25, .min = 30, .hr = 4,
        .date = 18, .month = 3, .year = 2025
    };

    ret = ds3231_update_date(new_time);
    ret = ds3231_update_time(new_time);
    if (ret != ESP_OK) ESP_LOGE(TAG, "DS3231 ERROR: TIME_SET");
    
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
    float bh1750_data = (readings[0] << 8 | readings[1]) / 1.2;
    snprintf(buff, sizeof(buff), "BH1750 %.2f", bh1750_data);
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
    snprintf(buff, sizeof(buff), "prox %u, als %u", ps, als);
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

    // Method 1: Direct clear channel conversion: 0.0576 * clear;
    // Method 2: RGB coefficients (CIE 1931): (0.2126 * red) + (0.7152 * green) + (0.0722 * blue);
    snprintf(buff, sizeof(buff), "ps %u, w %u, r %u, g %u, b %u", prox, clear, red, green, blue);
    ssd1306_print_str(buff, 6);

    return ret;
}

//! MAX44009
static esp_err_t max4400_get_reading() {
    esp_err_t ret;

    uint8_t reading[2];
    ret = i2c_write_read_command(max4400, 0x03, reading, sizeof(reading));

    uint8_t exponent = (reading[0] >> 4) & 0x0F;      // Extract exponent (bits 7-4)
    uint8_t mantissa = ((reading[0] & 0x0F) << 4) |   // Combine mantissa bits:
                      ((reading[1] >> 4) & 0x0F);     // high_byte[3:0] + low_byte[7:4]
    float lux = mantissa * 0.045 * (1 << exponent);   // Final lux calculation

    char buff[32];
    snprintf(buff, sizeof(buff), "lux %.2f", lux);
    // ssd1306_print_str(buff, 7);

    return ret;
}

//! VL53LOX
static esp_err_t vl53lox_get_reading() {
    esp_err_t ret;

    uint8_t status;
    ret = i2c_write_read_byte(vl53lox, 0x13, &status);    // Get STATUS REG

    // if (status & 0x07) {
        uint8_t reading[2];
        ret = i2c_write_read_command(vl53lox, 0x14, reading, sizeof(reading));
        uint8_t distance = (reading[0] << 8) | reading[1];

        char buff[32];
        snprintf(buff, sizeof(buff), "dist: %u", distance);
        // ssd1306_print_str(buff, 7);
    
    // }

    return ret;
}

//! MPU6050
static esp_err_t mpu6050_get_reading() {
    esp_err_t ret;

    uint8_t buff[6];
    ret = i2c_write_read_command(mpu6050, 0x3B, buff, sizeof(buff));

    int16_t accel_x = (buff[0] << 8) | buff[1];
    int16_t accel_y = (buff[2] << 8) | buff[3];
    int16_t accel_z = (buff[4] << 8) | buff[5];

    char str_buff[52];
    snprintf(str_buff, sizeof(str_buff), "x %u, y %u, z %u", accel_x, accel_y, accel_z);
    ssd1306_print_str(str_buff, 4);

    return ret;
}

//! INA219
static esp_err_t ina219_get_reading() {
    esp_err_t ret;

    uint8_t buff[2];
    ret = i2c_write_read_command(ina219, 0x01, buff, sizeof(buff));
    uint16_t shunt = (buff[0] << 8) | buff[1];

    ret = i2c_write_read_command(ina219, 0x02, buff, sizeof(buff));
    uint16_t bus = (buff[0] << 8) | buff[1];
    uint16_t bus_mV = (bus >> 3) * 4;       // convert to mV

    ret = i2c_write_read_command(ina219, 0x03, buff, sizeof(buff));
    uint16_t power = (buff[0] << 8) | buff[1];

    ret = i2c_write_read_command(ina219, 0x04, buff, sizeof(buff));
    int16_t current = (buff[0] << 8) | buff[1];

    char str_buff[52];
    snprintf(str_buff, sizeof(str_buff),
                "sh %hu, bus %hu, cur %hd, p %hu", 
                shunt, bus_mV, current, power);
    ssd1306_print_str(str_buff, 7);

    return ret;
}


//! DS3231
static esp_err_t ds3231_read_time(ds3231_dateTime_t *datetime) {
    uint8_t data[7];
    esp_err_t ret = i2c_write_read_command(ds3231, 0x00, data, sizeof(data));
    datetime->sec   = bcd_to_decimal(data[0]);
    datetime->min   = bcd_to_decimal(data[1]);
    datetime->hr    = bcd_to_decimal(data[2]);
    datetime->date  = bcd_to_decimal(data[4]);
    datetime->month = bcd_to_decimal(data[5] & 0x1F);   // Mask out the century bit
    datetime->year  = bcd_to_decimal(data[6]) + 2000;
    return ret;
}

uint64_t ds3231_timeRef = 0;

static esp_err_t ds3231_get_reading(uint64_t current_time) {
    if (current_time - ds3231_timeRef < 1000000) return ESP_FAIL;
    ds3231_timeRef = current_time;

    ds3231_dateTime_t read_dt;
    ds3231_read_time(&read_dt);

    char buff[32];
    snprintf(buff, sizeof(buff), "%u/%u/%u %u:%u:%u", 
        read_dt.month, read_dt.date, read_dt.year,
        read_dt.hr, read_dt.min, read_dt.sec);
    ssd1306_print_str(buff, 1);

    return ESP_OK;
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
    
        snprintf(buff, sizeof(buff), "Temp %.2f, hum %.2f", temp, hum);
        ssd1306_print_str(buff, 3);
        sht31_wait_time = 30000;
    }

    return ret;
}

void i2c_sensor_readings(uint64_t current_time) {
    sht31_get_readings(current_time);
    ds3231_get_reading(current_time);

    if (current_time - interval_ref < 300000) return;
    interval_ref = current_time;

    bh1750_get_reading();
    ap3216_get_reading();
    apds9960_get_reading();
    max4400_get_reading();
    vl53lox_get_reading();
    mpu6050_get_reading();
    ina219_get_reading();
}

void display_push_pixel(uint8_t y, uint8_t color) {
    // ssd1306_push_pixel(y, color);
}