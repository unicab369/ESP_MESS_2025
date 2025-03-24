#include "display.h"

#include "mod_ssd1306.h"
#include "driver/i2c.h"

#include "esp_log.h"
#include "esp_err.h"
#include "mod_i2c.h"
#include "mod_bitmap.h"

static const char *TAG = "I2C_DEVICE";


void display_setup(uint8_t scl_pin, uint8_t sda_pin) {
    esp_err_t ret = i2c_setup(scl_pin, sda_pin);

    ssd1306_setup(0x3C);
    ssd1306_print_str("Hello Bee", 0);
}

//# TODO: char_spacing crash above value 1
void display_spi_setup(uint8_t rst, M_Spi_Conf *conf) {
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
        .text = "Hello Bee! What is Thy bidding, my master? Tell me!"
                "\nTomorrow is another day!"
                "\n\nThis is a new line. Making a new line."
    };

    st7735_draw_text(&tft_text, conf);
}

void display_print_str(const char *str, uint8_t line) {
    ssd1306_print_str(str, line);
}
