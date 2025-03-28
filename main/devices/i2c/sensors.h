#include <unistd.h>
#include <stdint.h>

#include "driver/i2c.h"
#include "mod_i2c.h"

#include "esp_log.h"
#include "esp_err.h"

typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hr;
    uint8_t day;        // 1: Monday to 7: Sunday
    uint8_t month;
    uint16_t year;
    uint8_t date;
} ds3231_dateTime_t;


typedef struct {
    void (*on_handle_bh1750)(float reading);
    void (*on_handle_ap3216)(uint16_t ps, uint16_t als);
    void (*on_handle_apds9960)(uint8_t prox, uint16_t clear,
                            uint16_t red, uint16_t green, uint16_t blue);
    void (*on_handle_max4400)(float lux);
    void (*on_handle_vl53lox)(uint8_t distance);
    void (*on_handle_mpu6050)(int16_t accel_x, int16_t accel_y, int16_t accel_z);
    void (*on_handle_ina219)(int16_t shunt, int16_t bus, int16_t current, int16_t power);
    void (*on_handle_ds3231)(ds3231_dateTime_t *datetime);
    void (*on_handle_sht31)(float temp, float hum);
} M_Device_Handlers;

typedef struct {
    M_I2C_Device *ssd1306;
    M_I2C_Device *bh1750;
    M_I2C_Device *sht31;          // address 0x44 or 0x45
    M_I2C_Device *ap3216;
    M_I2C_Device *apds9960;
    M_I2C_Device *max4400;
    M_I2C_Device *vl53lox;
    M_I2C_Device *mpu6050;
    M_I2C_Device *ina219;
    M_I2C_Device *ds3231;
    M_Device_Handlers *handlers;
    i2c_port_t port;
} M_I2C_Devices_Set;


void i2c_devices_setup(M_I2C_Devices_Set *devs_set, uint8_t port);
void i2c_sensor_readings(M_I2C_Devices_Set *devs_set, uint64_t current_time);

esp_err_t ds3231_update_time(M_I2C_Device *device, ds3231_dateTime_t *datetime);
esp_err_t ds3231_update_date(M_I2C_Device *device, ds3231_dateTime_t *datetime);