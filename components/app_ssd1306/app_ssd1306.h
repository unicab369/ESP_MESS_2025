#include <stdint.h>

void ssd1306_setup(uint8_t scl_pin, uint8_t sda_pin, uint8_t address);
void ssd1306_display_str(const char *str, uint8_t line);
void ssd1306_display_str_at(const char *str, uint8_t page, uint8_t column);
void ssd1306_clear_all(void);
int do_i2cdetect_cmd(uint8_t scl_pin, uint8_t sda_pin);