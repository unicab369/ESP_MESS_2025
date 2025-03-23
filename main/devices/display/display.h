
#include <unistd.h>
#include <stdint.h>
#include "esp_err.h"

#include "mod_st7735.h"
// #include "mod_spi.h"

void display_setup(uint8_t scl_pin, uint8_t sda_pin);
void display_print_str(const char *str, uint8_t line);
void display_spi_setup(uint8_t rst, M_Spi_Conf *conf);