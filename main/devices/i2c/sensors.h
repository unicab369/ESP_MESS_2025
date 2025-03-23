#include <unistd.h>
#include <stdint.h>
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
    void (*on_handle_sht31)(float temp, float hum)
} M_Sensors_Interface;

void i2c_sensors_setup(M_Sensors_Interface *intf);
void i2c_sensor_readings(uint64_t current_time);

esp_err_t ds3231_update_time(ds3231_dateTime_t *datetime);
esp_err_t ds3231_update_date(ds3231_dateTime_t *datetime);