#include "mod_st7735.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mod_bitmap.h"

#define ST7735_WIDTH 160
#define ST7735_HEIGHT 80
#define DISPLAY_NUM_PIXELS (ST7735_WIDTH * ST7735_HEIGHT)

uint16_t tft_buffer[DISPLAY_NUM_PIXELS];

void st7735_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, M_Spi_Conf *conf) {
    mod_spi_cmd(0x2A, conf);              // ST7735_CASET
    uint8_t data[] = {0x00, x0, 0x00, x1};
    mod_spi_data(data, 4, conf);

    mod_spi_cmd(0x2B, conf);              // ST7735_RASET
    data[1] = y0;
    data[3] = y1;
    mod_spi_data(data, 4, conf);

    mod_spi_cmd(0x2C, conf);              // ST7735_RAMWR
}

void st7735_flush_framebuffer(M_Spi_Conf *conf) {
    st7735_set_address_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1, conf);

    // Send the entire framebuffer
    mod_spi_data((uint8_t *)tft_buffer, DISPLAY_NUM_PIXELS * 2, conf);
}

void st7735_fill_screen(uint16_t color, M_Spi_Conf *conf) {
    // Fill the framebuffer with the specified color
    for (int i = 0; i < DISPLAY_NUM_PIXELS; i++) {
        tft_buffer[i] = color;
    }

    // Send the entire framebuffer to the display
    st7735_flush_framebuffer(conf);
}


esp_err_t st7735_init(uint8_t rst, M_Spi_Conf *conf) {
    esp_err_t ret = gpio_set_direction(rst, GPIO_MODE_OUTPUT);
    // Reset the display
    gpio_set_level(rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Send initialization commands
    mod_spi_cmd(0x01, conf); // Software reset
    vTaskDelay(pdMS_TO_TICKS(120));

    mod_spi_cmd(0x11, conf); // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));
    
    mod_spi_cmd(0x3A, conf); // Set color mode
    mod_spi_data((uint8_t[]){0x05}, 1, conf); // 16-bit color

    mod_spi_cmd(0x29, conf); // Display on

    st7735_fill_screen(0xF800, conf);

    return ret;
}



void st7735_draw_char(uint8_t x, uint8_t y, char c, uint16_t color, uint16_t bg_color, M_Spi_Conf *conf) {
    const uint8_t *char_data = FONT_7x5[c - 32];
    uint8_t tft_frame_buffer[5 * 7 * 2];    // Frame buffer for a 5x7 character (2 bytes per pixel in RGB565)
    int buffer_index = 0;

    for (int j = 0; j < 7; j++) { // Rows
        for (int i = 0; i < 5; i++) { // Columns
            if (char_data[i] & (1 << j)) {
                // Pixel is part of the character (foreground color)
                tft_frame_buffer[buffer_index++] = color >> 8;   // High byte of RGB565
                tft_frame_buffer[buffer_index++] = color & 0xFF; // Low byte of RGB565
            } else {
                // Pixel is not part of the character (background color)
                tft_frame_buffer[buffer_index++] = bg_color >> 8;   // High byte of RGB565
                tft_frame_buffer[buffer_index++] = bg_color & 0xFF; // Low byte of RGB565
            }
        }
    }

    // Set the address window to cover the character area
    st7735_set_address_window(x, y, x + 4, y + 6, conf); // Character is 5x7 pixels

    // Send the frame buffer to the display in one transaction
    mod_spi_data(tft_frame_buffer, sizeof(tft_frame_buffer), conf);
}

void st7735_draw_text(uint8_t x, uint8_t y, const char* str, uint16_t color, M_Spi_Conf *conf) {
    while (*str) {
        st7735_draw_char(x, y, *str++, color, 0xF800, conf);
        x += 6; // Move to next character position
    }
}


