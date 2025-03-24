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


#define LOG_BUFFER_CONTENT 1
#define MAX_CHAR_COUNT 10

typedef struct {
    uint16_t line_idx;          // Current line index
    uint8_t current_x;          // Current x position
    uint8_t current_y;          // Current y position
    
    uint8_t font_width;         // Font width
    uint8_t font_height;        // Font height
    uint8_t font_spacing;       // Font spacing

    uint8_t char_buff[MAX_CHAR_COUNT]; // Buffer to store accumulated character data
    uint8_t char_count;    // Number of characters accumulated
    uint8_t x0;     // Starting x position of the current buffer
} M_Render_State;

esp_err_t st7735_init(uint8_t rst, M_Spi_Conf *conf);
void st7735_draw_text(M_TFT_Text *model, M_Spi_Conf *config);