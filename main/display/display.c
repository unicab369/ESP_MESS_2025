#include "display.h"

#include "app_ssd1306.h"



void display_setup(uint8_t scl_pin, uint8_t sda_pin, uint8_t address) {
    ssd1306_setup(scl_pin, sda_pin, address);
    ssd1306_print_str("Hello Bee", 0);
}

void display_print_str(const char *str, uint8_t line) {
    ssd1306_print_str(str, line);
}

