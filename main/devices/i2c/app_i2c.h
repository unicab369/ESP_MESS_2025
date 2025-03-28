
#include <unistd.h>
#include <stdint.h>

void ssd1306_set_printMode(uint8_t direction);
void app_i2c_setup(M_I2C_Devices_Set *devs_set, uint8_t scl_pin, uint8_t sda_pin);
void app_i2c_task(uint64_t current_time, M_I2C_Devices_Set *devs_set);