#include <stdint.h>
#include <stdbool.h>

void ssd1306_setup(uint8_t address);
void ssd1306_print_str(const char *str, uint8_t line);
void ssd1306_print_str_at(const char *str, uint8_t page, uint8_t column, bool clear);
void ssd1306_clear_all(void);

void ssd1306_push_pixel(uint8_t y, uint8_t color);
int do_i2cdetect_cmd(uint8_t scl_pin, uint8_t sda_pin);