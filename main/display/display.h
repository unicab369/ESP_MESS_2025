
#include <unistd.h>
#include <stdint.h>
#include "esp_err.h"

void display_setup(uint8_t scl_pin, uint8_t sda_pin);
void display_print_str(const char *str, uint8_t line);
esp_err_t print_bh1750_readings();
void i2c_sensor_readings(uint64_t current_time);
void display_push_pixel(uint8_t y, uint8_t color);