
#include <unistd.h>
#include <stdint.h>
#include "esp_err.h"

#include "st7735_shape.h"

void display_print_str(const char *str, uint8_t line);
void display_spi_setup(uint8_t rst, uint8_t busy, M_Spi_Conf *conf);