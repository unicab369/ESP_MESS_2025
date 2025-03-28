
#include <unistd.h>
#include <stdint.h>

void ssd1306_set_printMode(uint8_t direction);
void app_i2c_setup(uint8_t scl_pin, uint8_t sda_pin);