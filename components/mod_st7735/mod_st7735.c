#include "mod_st7735.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mod_bitmap.h"

#define ST7735_SIZE_1p8IN 1

#ifdef ST7735_SIZE_1p8IN
    #define ST7735_WIDTH 128
    #define ST7735_HEIGHT 160
#else
    #define ST7735_WIDTH 80
    #define ST7735_HEIGHT 160
#endif

#define DISPLAY_NUM_PIXELS (ST7735_WIDTH * ST7735_HEIGHT)

static uint16_t tft_buffer[DISPLAY_NUM_PIXELS];
static uint16_t BACKGROUND_COLOR = 0x0000;

void st7735_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, M_Spi_Conf *conf) {           
    uint8_t data[] = {0x00, x0, 0x00, x1};
    //! ST7735_CASET
    mod_spi_cmd(0x2A, conf);   
    mod_spi_data(data, 4, conf);

    data[1] = y0;
    data[3] = y1;
    //! ST7735_RASET
    mod_spi_cmd(0x2B, conf);
    mod_spi_data(data, 4, conf);

    //! ST7735_RAMWR
    mod_spi_cmd(0x2C, conf);
}

void st7735_fill_screen(uint16_t color, M_Spi_Conf *conf) {
    //! Set the address window to cover the entire screen
    st7735_set_address_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1, conf);

    //! Precompute the high and low bytes of the color
    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;

    const size_t CHUNK_SIZE = 2000;
    uint8_t chunk[CHUNK_SIZE]; // Buffer to hold the chunk

    //! Calculate the total number of pixels
    size_t total_pixels = ST7735_WIDTH * ST7735_HEIGHT;

    //! Fill the screen in chunks
    size_t pixels_sent = 0;
    while (pixels_sent < total_pixels) {
        //! Determine the number of pixels to send in this chunk
        size_t pixels_in_chunk = (total_pixels - pixels_sent > CHUNK_SIZE / 2)
                                ? CHUNK_SIZE / 2
                                : (total_pixels - pixels_sent);

        //! Fill the chunk with the color data
        for (size_t i = 0; i < pixels_in_chunk; i++) {
            chunk[2 * i] = color_high;   // High byte
            chunk[2 * i + 1] = color_low; // Low byte
        }

        //! Send the chunk to the display
        mod_spi_data(chunk, pixels_in_chunk * 2, conf);

        //! Update the number of pixels sent
        pixels_sent += pixels_in_chunk;
    }
}

esp_err_t st7735_init(uint8_t rst, M_Spi_Conf *conf) {
    //! Set the RST pin
    esp_err_t ret = gpio_set_direction(rst, GPIO_MODE_OUTPUT);

    //! Reset the display
    gpio_set_level(rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    //! Send initialization commands
    mod_spi_cmd(0x01, conf); // Software reset
    vTaskDelay(pdMS_TO_TICKS(120));
    mod_spi_cmd(0x11, conf); // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));
    
    //! Set color mode
    mod_spi_cmd(0x3A, conf);
    mod_spi_data((uint8_t[]){0x05}, 1, conf); // 16-bit color

    //! Start display
    mod_spi_cmd(0x29, conf); // Display on

    //! Fill display color
    st7735_fill_screen(BACKGROUND_COLOR, conf);

    return ret;
}

void st7735_draw_text(uint8_t x, uint8_t y, const char* str, uint16_t color, bool is_wrap, M_Spi_Conf *conf) {
    //! Save the initial x position for wrapping
    uint8_t current_x = x;

    while (*str) {
        const uint8_t *char_data = FONT_7x5[*str - 32];
        //! buffer contains char Width * Height * 2 (Color)
        uint8_t tft_frame_buffer[5 * 7 * 2];
        int buff_idx = 0;

        //! Handle Wraping
        if (current_x + 5 > ST7735_WIDTH) {
            if (is_wrap) {
                //! Handle Wrap: to the next line
                current_x = x; // Reset x to the start position
                y += 8; // Move y to the next line (7 pixels for the character + 1 pixel spacing)
                if (y + 7 > ST7735_HEIGHT) break;
            } else {
                //! Handle Trunction: draw as normal
                break;
            }
        }

        //! Render the character into the frame buffer
        for (int j = 0; j < 7; j++) { // Rows
            for (int i = 0; i < 5; i++) { // Columns
                if (char_data[i] & (1 << j)) {
                    //! Pixel is part of the character (foreground color)
                    tft_frame_buffer[buff_idx++] = color >> 8;   // High byte of RGB565
                    tft_frame_buffer[buff_idx++] = color & 0xFF; // Low byte of RGB565
                } else {
                    //! Pixel is not part of the character (background color)
                    tft_frame_buffer[buff_idx++] = BACKGROUND_COLOR >> 8;   // High byte of RGB565
                    tft_frame_buffer[buff_idx++] = BACKGROUND_COLOR & 0xFF; // Low byte of RGB565
                }
            }
        }

        //! Set the address window to cover the character area
        st7735_set_address_window(current_x, y, current_x + 4, y + 6, conf); // Character is 5x7 pixels

        //! Send the frame buffer to the display in one transaction
        mod_spi_data(tft_frame_buffer, sizeof(tft_frame_buffer), conf);

        //! Move to the next character position
        current_x += 6; // 5 pixels for the character + 1 pixel spacing
        str++; // Move to the next character in the string
    }
}
