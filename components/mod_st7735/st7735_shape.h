
#include <unistd.h>
#include <stdint.h>
#include "esp_err.h"

#include "mod_st7735.h"

void st7735_draw_line(int16_t x0, int16_t y0, int16_t x1, 
    int16_t y1, uint16_t color, M_Spi_Conf *config);

void st7735_draw_horLine(
    uint16_t y, uint16_t x0, uint16_t x1, 
    uint16_t color, uint8_t thickness, M_Spi_Conf *config
);

void st7735_draw_verLine(
    uint16_t x, uint16_t y0, uint16_t y1,
    uint16_t color, uint8_t thickness,
    M_Spi_Conf *config
);