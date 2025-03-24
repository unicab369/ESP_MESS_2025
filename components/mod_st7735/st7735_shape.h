
#include <unistd.h>
#include <stdint.h>
#include "esp_err.h"

#include "mod_st7735.h"

void st7735_draw_line(int16_t x0, int16_t y0, int16_t x1, 
    int16_t y1, uint16_t color, M_Spi_Conf *config);
