
#include <unistd.h>
#include <stdint.h>

#include "i2c/sensors.h"


void app_serial_setMode(uint8_t direction);
void app_serial_i2c_setup(M_I2C_Devices_Set *devs_set, uint8_t scl_pin, uint8_t sda_pin);
void app_serial_i2c_task(uint64_t current_time, M_I2C_Devices_Set *devs_set);