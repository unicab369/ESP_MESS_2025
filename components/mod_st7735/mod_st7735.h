#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

#include "mod_spi.h"

esp_err_t st7735_init(uint8_t rst, M_Spi_Conf *conf);
void st7735_draw_text(uint8_t x, uint8_t y, const char* str, uint16_t color, bool is_wrap, M_Spi_Conf *conf);