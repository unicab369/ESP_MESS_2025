
#include <unistd.h>
#include <stdint.h>
#include "esp_err.h"

#include "st7735_shape.h"

void display_setup(uint8_t scl_pin, uint8_t sda_pin);
void display_print_str(const char *str, uint8_t line);
void display_spi_setup(uint8_t rst, M_Spi_Conf *conf);