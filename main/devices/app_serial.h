
#include <unistd.h>
#include <stdint.h>

#include "i2c/sensors.h"


void app_serial_setMode(uint8_t direction);
void app_serial_i2c_setup(uint8_t scl_pin, uint8_t sda_pin, uint8_t port);
void app_serial_i2c_task(uint64_t current_time);