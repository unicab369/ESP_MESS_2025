#include "spi_devices.h"

#include "mod_spi.h"

#include "mod_sd.h"

#include "st7735_shape.h"

#include "mod_bitmap.h"


M_Spi_Conf spi_config;


void spi_devices_setup(
    uint8_t host, 
    int8_t mosi_pin, int8_t miso_pin, int8_t clk_pin, int8_t cs_pin,
    int8_t dc_pin, int8_t rst_pin,
    int8_t cs_x0, int8_t cs_x1
) {
    spi_config.host = SPI2_HOST,
    spi_config.mosi = mosi_pin,
    spi_config.miso = miso_pin,
    spi_config.clk = clk_pin,
    spi_config.cs = cs_pin,

    spi_config.dc = dc_pin;
    spi_config.rst = rst_pin;

    //# IMPORTANTE: Reset CS pins
    mod_spi_setup_cs(cs_pin);
    mod_spi_setup_cs(cs_x0);
    mod_spi_setup_cs(cs_x1);

    //# Init SPI peripheral
    esp_err_t ret = mod_spi_init(&spi_config);

    if (ret == ESP_OK) {
        storage_sd_config_t sd_config = {
            .mmc_enabled = false,
    
            //! NOTE: for MMC D3 or CS needs to be pullup if not used otherwise it will go into SPI mode
            // .mmc = {
            //     .enable_width4 = false,
            //     .clk = MMC_CLK,
            //     .di = MMC_DI,
            //     .d0 = MMC_D0
            // },
        };
    
        //# SD Card
        mod_sd_spi_config(spi_config.cs);
        mod_sd_test();

        //# IMPORTANTE: Switch CS pins
        mod_spi_switch_cs(spi_config.cs, cs_x0);
        
        //# ST7735
        st7735_init(&spi_config);

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
    
        st7735_draw_text(&tft_text, &spi_config);
        st7735_draw_horLine(80, 10, 100, 0xF800, 3, &spi_config);
        st7735_draw_verLine(80, 10, 100, 0xF800, 3, &spi_config);
        st7735_draw_rectangle(20, 20, 50, 50, 0xAA00, 3, &spi_config);
        // st7735_draw_line(0, 0, 80, 150, 0x00CC, &spi_config);
    }
}