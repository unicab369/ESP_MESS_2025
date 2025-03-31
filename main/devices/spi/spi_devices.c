#include "spi_devices.h"


#include "mod_sd.h"
#include "st7735_shape.h"
#include "mod_bitmap.h"

M_Spi_Conf spi_config;


// SPI loads division:
// CH1 - SD Card & ST7735 display
// CH2 - LoRa Module

void spi_setup_sdCard(uint8_t host, uint8_t cs_pin) {
    //# SD Card
    //! NOTE: for MMC D3 or CS needs to be pullup if not used otherwise it will go into SPI mode
    mod_sd_spi_config(host, cs_pin);
    mod_sd_test();
}

void spi_setup_st7735(M_Spi_Conf *config, uint8_t st7735_cs_pin) {
    //# IMPORTANTE: Switch CS pins
    mod_spi_setup_cs(st7735_cs_pin);
    mod_spi_switch_cs(config->cs, st7735_cs_pin);

    //# ST7735
    st7735_init(config);

    M_TFT_Text tft_text = {
        .x = 0, .y = 0,
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

    st7735_draw_text(&tft_text, config);
    st7735_draw_horLine(80, 10, 100, 0xF800, 3, config);
    st7735_draw_verLine(80, 10, 100, 0xF800, 3, config);
    st7735_draw_rectangle(20, 20, 50, 50, 0xAA00, 3, config);
    // st7735_draw_line(0, 0, 80, 150, 0x00CC, config);
}