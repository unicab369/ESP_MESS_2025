#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

#include "mod_spi.h"


typedef struct {
    uint8_t x;
    uint8_t y;
    uint16_t color;
    uint8_t page_wrap;
    uint8_t word_wrap;

    const uint8_t *font;    // Pointer to the font data
    uint8_t font_width;     // Width of each character in the font
    uint8_t font_height;    // Height of each character in the font
    uint8_t char_spacing;   // Spacing between characters (default: 1 pixel)
    const char* text;
} M_TFT_Text;

esp_err_t st7735_init(uint8_t rst, M_Spi_Conf *conf);
void st7735_draw_text(M_TFT_Text *model, M_Spi_Conf *config);