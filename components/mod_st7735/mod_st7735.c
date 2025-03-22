#include "mod_st7735.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ST7735_DISPLAY_WIDTH  128
#define ST7735_DISPLAY_HEIGHT 160

#define FONT_WIDTH 5
#define FONT_HEIGHT 7

static spi_device_handle_t spi;
static uint8_t dc_pin;
static esp_err_t current_err;

esp_err_t st7735_send_cmd(uint8_t cmd) {
    spi_transaction_t t = {
        .length = 8, 
        .tx_buffer = &cmd,
    };
    gpio_set_level(dc_pin, 0); // Command mode

    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Send command Failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t st7735_send_data(uint8_t *data, uint16_t len) {
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    gpio_set_level(dc_pin, 1); // Data mode
    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Transmit data Failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

void st7735_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    st7735_send_cmd(0x2A);              // ST7735_CASET
    uint8_t data[] = {0x00, x0, 0x00, x1};
    st7735_send_data(data, 4);

    st7735_send_cmd(0x2B);              // ST7735_RASET
    data[1] = y0;
    data[3] = y1;
    st7735_send_data(data, 4);

    st7735_send_cmd(0x2C);              // ST7735_RAMWR
}

void st7735_fill_screen(uint16_t color) {
    st7735_set_address_window(0, 0, ST7735_DISPLAY_WIDTH - 1, ST7735_DISPLAY_HEIGHT - 1);
    
    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;
    uint8_t data[] = {color_high, color_low};

    for (int i = 0; i < ST7735_DISPLAY_WIDTH * ST7735_DISPLAY_HEIGHT; i++) {
        st7735_send_data(data, 2);
    }
}

esp_err_t st7735_init(uint8_t mosi, uint8_t clk, uint8_t dc, uint8_t rst) {
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = -1, // No MISO
        .mosi_io_num = mosi,
        .sclk_io_num = clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };

    esp_err_t ret = spi_bus_initialize(SPI3_HOST, &buscfg, 0);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure SPI device for ST7735
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 27000000 ,        // 40 MHz
        .mode = 0,                          // SPI mode 0
        .queue_size = 1,
    };
    ret = spi_bus_add_device(SPI3_HOST, &devcfg, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize GPIO pins
    gpio_set_direction(dc, GPIO_MODE_OUTPUT);
    gpio_set_direction(rst, GPIO_MODE_OUTPUT);

    // Reset the display
    gpio_set_level(rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    dc_pin = dc;

    // Send initialization commands
    st7735_send_cmd(0x01); // Software reset
    vTaskDelay(pdMS_TO_TICKS(120));

    st7735_send_cmd(0x11); // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));
    
    st7735_send_cmd(0x3A); // Set color mode
    st7735_send_data((uint8_t[]){0x05}, 1); // 16-bit color

    st7735_send_cmd(0x29); // Display on

    st7735_fill_screen(0xF800);

    current_err = ret;
    return ret;
}

static const uint8_t font[95][FONT_HEIGHT] = {
    // Space (ASCII 32)
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x1F, 0x10, 0x08, 0x04, 0x02, 0x01, 0x1F},
};

void st7735_draw_pixel(uint8_t x, uint8_t y, uint16_t color) {
    st7735_set_address_window(x, y, x, y);
    uint8_t data[] = {color >> 8, color & 0xFF};
    st7735_send_data(data, 2);
}

void st7735_draw_char(uint8_t x, uint8_t y, char c, uint16_t color) {
    const uint8_t *char_data = font[0];

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 7; j++) {
            if (char_data[i] & (1 << j)) {
                st7735_draw_pixel(x + i, y + j, color);
            }
        }
    }
}

void st7735_draw_text(uint8_t x, uint8_t y, const char* str, uint16_t color) {

    while (*str) {
        st7735_draw_char(x, y, *str++, color);
        x += 6; // Move to next character position
    }
}

