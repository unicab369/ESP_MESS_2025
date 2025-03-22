#include "mod_st7735.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define FONT_WIDTH 5
#define FONT_HEIGHT 7

struct {
    uint8_t width;
    uint8_t height;
} ST7735_DISPLAY_18IN = { 128, 160 };

struct {
    uint8_t width;
    uint8_t height;
} ST7735_DISPLAY_096IN = { 160, 80 };

uint16_t framebuffer[160 * 80];

static esp_err_t current_err;

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

void st7735_fill_screen(uint16_t color, M_Spi_Conf *conf) {
    st7735_set_address_window(0, 0, ST7735_DISPLAY_096IN.width - 1, ST7735_DISPLAY_096IN.height - 1, conf);
    
    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;
    uint8_t data[] = {color_high, color_low};

    for (int i = 0; i < ST7735_DISPLAY_096IN.width * ST7735_DISPLAY_096IN.height; i++) {
        mod_spi_data(data, 2, conf);
    }
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

    current_err = ret;
    return ret;
}

static const uint8_t font[95][FONT_WIDTH] = {
    // Space (ASCII 32)
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
};

void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color, M_Spi_Conf *conf) {
    st7735_set_address_window(x, y, x, y, conf);
    uint8_t data[] = {color >> 8, color & 0xFF};
    mod_spi_data(data, 2, conf);
}

void st7735_draw_char(uint8_t x, uint8_t y, char c, uint16_t color, M_Spi_Conf *conf) {
    const uint8_t *char_data = font[0];

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 7; j++) {
            if (char_data[i] & (1 << j)) {
                st7735_draw_pixel(x + i, y + j, color, conf);
            }
        }
    }
}

void st7735_draw_text(uint8_t x, uint8_t y, const char* str, uint16_t color, M_Spi_Conf *conf) {
    while (*str) {
        st7735_draw_char(x, y, *str++, color, conf);
        x += 6; // Move to next character position
    }
}

