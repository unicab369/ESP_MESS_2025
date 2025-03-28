#include "sensors.h"


static const char *TAG = "I2C_SENSORS";

#define BCD_TO_DECIMAL(bcd)         ((bcd >> 4) * 10 + (bcd & 0x0F))            // Convert BCD to decimal
#define DECIMAL_TO_BCD(decimal)     (((decimal / 10) << 4) | (decimal % 10))    // Convert decimal to BCD

typedef enum {
    BH1750_CONTINUOUS_1LX       = 0x10,     // 1 lux resolution 120ms
    BH1750_CONTINUOUS_HALFLX    = 0x11,     // .5 lux resolution 120ms
    BH1750_CONTINUOUS_4LX_RES   = 0x13,     // 4 lux resolution 16ms
    BH1750_ONETIME_1LX          = 0x20,     // 1 lux resolution 120ms
    BH1750_ONETIME_HALFLX       = 0x21,     // .5 lux resolution 120ms
    BH1750_ONETIME_4LX          = 0x23,     // 4 lux resolution 16ms

} bh1750_mode_t;


static void print_error(esp_err_t ret, const char *text) {
    if (ret == ESP_OK) {
        printf("%s started.\n", text);
    } else {
        ESP_LOGE(TAG, "%s ERROR.", text);
    }
}


void i2c_devices_setup(M_I2C_Devices_Set *devs_set, uint8_t port) {
    //! bh1750
    devs_set->bh1750 = i2c_device_create(port, 0x23);
    uint8_t data = 0x01;
    esp_err_t ret = i2c_write(devs_set->bh1750, &data, 1);           // BH1750 POWER_ON = 0x01, POWER_DOWN = 0x00
    print_error(ret, "BH1750");

    //! sht31
    devs_set->sht31 = i2c_device_create(port, 0x44);
    ret = i2c_write_register_byte(devs_set->sht31, 0x30, 0xA2);     // SOFT_RESET 0x30A2
    print_error(ret, "SHT31");

    //! AP3216
    devs_set->ap3216 = i2c_device_create(port, 0x1E);
    ret = i2c_write_register_byte(devs_set->ap3216, 0x00, 0x03);    // RESET
    print_error(ret, "AP3216");

    //! APDS9960
    uint8_t config[] = {
        0x80,           // ENABLE
        0x0F,           // Power ON, Proximity enable, ALS enable, Wait enable
        0x90,           // CONFIG2
        0x01,           // Proximity gain control (4x)
        0x8F, 0x20,     // Proximity pulse count (8 pulses)
        0x8E, 0x87      // Proximity pulse length (16us)
    };

    devs_set->apds9960 = i2c_device_create(port, 0x39);
    ret = i2c_write(devs_set->apds9960, config, sizeof(config));
    print_error(ret, "APDS9960");

    //! MAX44009
    devs_set->max4400 = i2c_device_create(port, 0x4A);       // AO to ground for ADDR 0x4B
    ret = i2c_write_register_byte(devs_set->max4400, 0x02, 0x80);       // 0x02 Config_Reg, 0x80 Config_value
    print_error(ret, "MAX44009");

    //! VL53LOX
    devs_set->vl53lox = i2c_device_create(port, 0x29);

    uint8_t model_id;
    ret = i2c_write_read_register(devs_set->vl53lox, 0xC0, &model_id, 1);
    ret = i2c_write_register_byte(devs_set->vl53lox, 0x00, 0x01);       // write command to START_RANGING REG
    print_error(ret, "VL53LOX");

    //! MPU6050
    devs_set->mpu6050 = i2c_device_create(port, 0x69);       // ADO HI 0x69, unconnect 0x68
    ret = i2c_write_register_byte(devs_set->mpu6050, 0x6B, 0x00);       // WAKEUP REG
    print_error(ret, "MPU6050");

    //! INA219
    devs_set->ina219 = i2c_device_create(port, 0x40);        // adjust A0 and A1 for 0x41, 0x44, 0x45
    uint8_t ina_config[2] = { 0x39, 0x9F };
    ret = i2c_write_register(devs_set->ina219, 0x00, ina_config, sizeof(ina_config));
    print_error(ret, "INA219");

    //! DS3231
    devs_set->ds3231 = i2c_device_create(port, 0x68);
    ds3231_dateTime_t new_time = {
        .sec = 25, .min = 30, .hr = 4,
        .date = 18, .month = 3, .year = 2025
    };

    if (ret == ESP_OK) {
        printf("DS3231 started.\n");
        ret = ds3231_update_date(devs_set->ds3231, &new_time);
        ret = ds3231_update_time(devs_set->ds3231, &new_time);
    } else {
        ESP_LOGE(TAG, "DS3231 ERROR: TIME_SET");
    }
}


//! DS3231
esp_err_t ds3231_update_time(M_I2C_Device *device, ds3231_dateTime_t *datetime) {
    uint8_t time[3] = { datetime->sec, datetime->min, datetime->hr };
    return i2c_write_register(device, 0x00, time, sizeof(time));
}

esp_err_t ds3231_update_date(M_I2C_Device *device, ds3231_dateTime_t *datetime) {
    esp_err_t ret = i2c_write_register_byte(device, 0x04, DECIMAL_TO_BCD(datetime->date));
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR WRITE_DATE DS3231");

    ret = i2c_write_register_byte(device, 0x05, DECIMAL_TO_BCD(datetime->month));
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR WRITE_MONTH DS3231");

    ret = i2c_write_register_byte(device, 0x06, DECIMAL_TO_BCD(datetime->year-2000));
    if (ret != ESP_OK) ESP_LOGE(TAG, "ERROR WRITE_YEAR DS3231");
    return ret;
}


//! DS3231
static esp_err_t ds3231_read_time(M_I2C_Device *device, ds3231_dateTime_t *datetime) {
    uint8_t data[7];
    esp_err_t ret = i2c_write_read_register(device, 0x00, data, sizeof(data));
    datetime->sec   = BCD_TO_DECIMAL(data[0]);
    datetime->min   = BCD_TO_DECIMAL(data[1]);
    datetime->hr    = BCD_TO_DECIMAL(data[2]);
    datetime->date  = BCD_TO_DECIMAL(data[4]);
    datetime->month = BCD_TO_DECIMAL(data[5] & 0x1F);   // Mask out the century bit
    datetime->year  = BCD_TO_DECIMAL(data[6]) + 2000;
    return ret;
}

uint64_t ds3231_timeRef = 0;

static esp_err_t ds3231_get_reading(M_I2C_Device *device, M_Device_Handlers *handlers,  uint64_t current_time) {
    if (current_time - ds3231_timeRef < 1000000) return ESP_FAIL;
    ds3231_timeRef = current_time;

    ds3231_dateTime_t read_dt;
    ds3231_read_time(device, &read_dt);
    handlers->on_handle_ds3231(&read_dt);

    return ESP_OK;
}

//! bh1750
static esp_err_t bh1750_get_reading(M_I2C_Device *device, M_Device_Handlers *handlers) {
    esp_err_t ret;

    uint8_t readings[2] = {0};
    ret = i2c_write_read_register(device, BH1750_CONTINUOUS_4LX_RES, readings, sizeof(readings));
    float lux = (readings[0] << 8 | readings[1]) / 1.2;
    handlers->on_handle_bh1750(lux);

    return ret;
}

//! AP3216
static esp_err_t ap3216_get_reading(M_I2C_Device *device, M_Device_Handlers *handlers) {
    uint8_t valHi; uint8_t valLo;
    esp_err_t ret = i2c_write_read_register(device, 0x0F, &valHi, 1);
    ret = i2c_write_read_register(device, 0x0E, &valLo, 1);

    int hiByte = valHi & 0b00111111; // PS HIGH_REG 6 bits
    int loBite = valLo & 0b00001111; // PS LOW_REG 4 bits
    uint16_t ps = (hiByte << 4) + loBite;

    ret = i2c_write_read_register(device, 0x0D, &valHi, 1);
    ret = i2c_write_read_register(device, 0x0C, &valLo, 1);
	uint16_t als = (valHi << 8) + valLo;
    handlers->on_handle_ap3216(ps, als);

    return ret;
}

//! ADPS9960
static esp_err_t apds9960_get_reading(M_I2C_Device *device, M_Device_Handlers *handlers) {
    esp_err_t ret;
    uint8_t prox;
    ret = i2c_write_read_register(device, 0x9C, &prox, 1);      // PROX DATA 0x9c

    // Clear Light LowByte
    uint8_t raw[8] = { 0 };
    ret = i2c_write_read_register(device, 0x94, raw, sizeof(raw));

    uint16_t clear  = (raw[1] << 8) | raw[0];
    uint16_t red    = (raw[3] << 8) | raw[2];
    uint16_t green  = (raw[5] << 8) | raw[4];
    uint16_t blue   = (raw[7] << 8) | raw[6];

    // Method 1: Direct clear channel conversion: 0.0576 * clear;
    // Method 2: RGB coefficients (CIE 1931): (0.2126 * red) + (0.7152 * green) + (0.0722 * blue);
    handlers->on_handle_apds9960(prox, clear, red, green, blue);

    return ret;
}

//! MAX44009
static esp_err_t max4400_get_reading(M_I2C_Device *device, M_Device_Handlers *handlers) {
    esp_err_t ret;

    uint8_t reading[2];
    ret = i2c_write_read_register(device, 0x03, reading, sizeof(reading));

    uint8_t exponent = (reading[0] >> 4) & 0x0F;      // Extract exponent (bits 7-4)
    uint8_t mantissa = ((reading[0] & 0x0F) << 4) |   // Combine mantissa bits:
                      ((reading[1] >> 4) & 0x0F);     // high_byte[3:0] + low_byte[7:4]
    float lux = mantissa * 0.045 * (1 << exponent);   // Final lux calculation
    handlers->on_handle_max4400(lux);

    return ret;
}

//! VL53LOX
static esp_err_t vl53lox_get_reading(M_I2C_Device *device, M_Device_Handlers *handlers) {
    esp_err_t ret;

    uint8_t status;
    ret = i2c_write_read_register(device, 0x13, &status, 1);    // Get STATUS REG

    // if (status & 0x07) {
        uint8_t reading[2];
        ret = i2c_write_read_register(device, 0x14, reading, sizeof(reading));
        uint8_t distance = (reading[0] << 8) | reading[1];
        handlers->on_handle_vl53lox(distance);
    // }

    return ret;
}

//! MPU6050
static esp_err_t mpu6050_get_reading(M_I2C_Device *device, M_Device_Handlers *handlers) {
    esp_err_t ret;

    uint8_t buff[6];
    ret = i2c_write_read_register(device, 0x3B, buff, sizeof(buff));

    int16_t accel_x = (buff[0] << 8) | buff[1];
    int16_t accel_y = (buff[2] << 8) | buff[3];
    int16_t accel_z = (buff[4] << 8) | buff[5];
    handlers->on_handle_mpu6050(accel_x, accel_y, accel_z);

    return ret;
}

//! INA219
static esp_err_t ina219_get_reading(M_I2C_Device *device, M_Device_Handlers *handlers) {
    esp_err_t ret;

    uint8_t buff[2];
    ret = i2c_write_read_register(device, 0x01, buff, sizeof(buff));
    int16_t shunt = (buff[0] << 8) | buff[1];

    ret = i2c_write_read_register(device, 0x02, buff, sizeof(buff));
    int16_t bus = (buff[0] << 8) | buff[1];
    int16_t bus_mV = (bus >> 3) * 4;       // convert to mV

    ret = i2c_write_read_register(device, 0x03, buff, sizeof(buff));
    int16_t power = (buff[0] << 8) | buff[1];

    ret = i2c_write_read_register(device, 0x04, buff, sizeof(buff));
    int16_t current = (buff[0] << 8) | buff[1];
    handlers->on_handle_ina219(shunt, bus_mV, current, power);

    return ret;
}

//! sht31
uint64_t sht31_wait_time = 30000;
uint64_t sht31_timeRef = 0;
uint64_t interval_ref = 0;

static esp_err_t sht31_get_readings(M_I2C_Device *device, M_Device_Handlers *handlers,  uint64_t current_time) {
    if (current_time - sht31_timeRef < sht31_wait_time) return ESP_FAIL;
    sht31_timeRef = current_time;

    esp_err_t ret;
    char buff[32];
    uint8_t sht_readings[6] = {0};

    if (sht31_wait_time == 30000) {
        uint8_t sht_cmd[2] = { 0x24, 0x00 };
        ret = i2c_write_register_byte(device, 0x24, 0x00);     // HIGHREP 0x2400
        if (ret != ESP_OK) return ret;
        sht31_wait_time = 300000;
    } else {
        ret = i2c_read(device, sht_readings, sizeof(sht_readings));
        if (ret != ESP_OK) return ret;
    
        uint16_t temp_raw = (sht_readings[0] << 8) | sht_readings[1];
        uint16_t hum_raw = (sht_readings[3] << 8) | sht_readings[4];
        float temp = -45.0f + 175.0f * ((float)temp_raw / 65535.0f);
        float hum = 100.0f * ((float)hum_raw / 65535.0f);
    
        handlers->on_handle_sht31(temp, hum);
        sht31_wait_time = 30000;
    }

    return ret;
}

void i2c_sensor_readings(M_I2C_Devices_Set *devs_set, uint64_t current_time) {
    sht31_get_readings(devs_set->sht31, devs_set->handlers, current_time);
    ds3231_get_reading(devs_set->ds3231, devs_set->handlers, current_time);

    if (current_time - interval_ref < 300000) return;
    interval_ref = current_time;

    bh1750_get_reading(devs_set->bh1750, devs_set->handlers);
    ap3216_get_reading(devs_set->ap3216, devs_set->handlers);
    apds9960_get_reading(devs_set->apds9960, devs_set->handlers);
    max4400_get_reading(devs_set->max4400, devs_set->handlers);
    vl53lox_get_reading(devs_set->vl53lox, devs_set->handlers);
    mpu6050_get_reading(devs_set->mpu6050, devs_set->handlers);
    ina219_get_reading(devs_set->ina219, devs_set->handlers);
}