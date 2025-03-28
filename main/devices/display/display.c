#include "display.h"

#include "mod_ssd1306.h"
#include "driver/i2c.h"

#include "esp_log.h"
#include "esp_err.h"
#include "mod_i2c.h"
#include "mod_bitmap.h"
#include "mod_epaper.h"

static const char *TAG = "I2C_DEVICE";

typedef enum {
    DISPLAY_I2C_CH0,    // SSD1306 on I2C0
    DISPLAY_I2C_CH1,
    DISPLAY_SPI_CH0,
    DISPLAY_SPI_CH1,
    DISPLAY_SPI_CH2
} E_DisplayID;

typedef struct {

} M_Display_Manager;

// typedef struct {
//     SSD1306_t *ssd1306;  // I2C display
//     ST7735_t  *st7735;   // SPI display
// } DisplayManager;

// DisplayManager displays = {
//     .ssd1306 = &dev1,
//     .st7735  = &dev2
// };



//# TODO: char_spacing crash above value 1
void display_spi_setup(uint8_t rst, uint8_t busy, M_Spi_Conf *conf) {
    st7735_init(rst, conf);

    M_TFT_Text tft_text = {
        .x = 0,
        .y = 0,
        .color = 0x00AA,
        .page_wrap = 1,
        .word_wrap = 1,

        .font = (const uint8_t *)FONT_7x5,      // Pointer to the font data
        .font_width = 5,                        // Font width
        .font_height = 7,                       // Font height
        .char_spacing = 1,                      // Spacing between characters
        .text = "What is Thy bidding, my master? Tell me!"
                "\nTomorrow is another day!"
                "\n\nThis is a new line. Continue with this line."
    };

    // st7735_draw_line(0, 0, 80, 150, 0x00CC, conf);

    // st7735_draw_text(&tft_text, conf);
    st7735_draw_horLine(80, 10, 100, 0xF800, 3, conf);
    st7735_draw_verLine(80, 10, 100, 0xF800, 3, conf);
    st7735_draw_rectangle(20, 20, 50, 50, 0xAA00, 3, conf);

    // ssd1683_setup(rst, busy, conf);
}

void display_print_str(const char *str, uint8_t line) {
    ssd1306_print_str(str, line);
}
