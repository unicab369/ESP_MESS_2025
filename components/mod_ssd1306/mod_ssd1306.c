#include "mod_ssd1306.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "mod_i2c.h"
#include "mod_utility.h"

// Display dimensions
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_MAX_CHAR (SSD1306_WIDTH/5)

// I2C Configuration
#define I2C_PORT I2C_NUM_0
#define SSD1306_PAGES 8
#define MAX_PAGE_INDEX (SSD1306_PAGES-1)

// Command constants
#define SSD1306_CMD 0x00
#define SSD1306_DATA 0x40


// 0x00 - Horizontal: Column pointer increments, wraps to next page after reaching end of line.
// 0x01 - Vertical: Page pointer increments, wraps to next column after reaching end of page.
// 0x02 - Page: Column pointer increments, stops after reaching end of page.
#define SSD1306_INITIAL_DISPLAY_MODE 0x00

// static const char *TAG = "SSD1306";
#define BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

static uint8_t buffer[BUFFER_SIZE]; // Buffer to hold pixel data
static uint8_t zero_buffer[SSD1306_WIDTH] = {0}; // Buffer of zeros (128 bytes)
#define END_COLUMN (SSD1306_WIDTH-1)

static i2c_device_t *ssd1306 = NULL;

uint8_t font7x5[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x00, 0x7F, 0x41, 0x41}, // [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 
    {0x41, 0x41, 0x7F, 0x00, 0x00}, // ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x00, 0x7F, 0x10, 0x28, 0x44}, // k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // ~
};


// Write command to SSD1306
static void send_cmd(uint8_t value) {
    uint8_t buff[2] = {SSD1306_CMD, value};
    i2c_write_register_byte(ssd1306, SSD1306_CMD, value);
}

// Write data to SSD1306
static void send_data(const uint8_t *data, size_t len) {
    i2c_write_register(ssd1306, SSD1306_DATA, data, len);
}

static void set_page(uint8_t page) {
    send_cmd(0xB0 | (page & 0x07)); // Set page address (0-7)
}

// Function to set the column address
static void set_column(uint8_t col) {
    send_cmd(0x00 | (col & 0x0F));       // Set lower column address
    send_cmd(0x10 | ((col >> 4) & 0x0F)); // Set higher column address
}

static void set_update_target(uint8_t start_page, uint8_t end_page) {
    send_cmd(0x21);         // Set column address
    send_cmd(0x00);         // Start column = 0
    send_cmd(END_COLUMN);   // End column = 127

    send_cmd(0x22);         // Set page address
    send_cmd(start_page);   // Start page
    send_cmd(end_page);     // End page
}

void ssd1306_clear_lines(uint8_t start_page, uint8_t end_page) {
    if (end_page>MAX_PAGE_INDEX) return;
    set_update_target(start_page, end_page);

    // Send zero buffer for each page
    for (uint8_t page = start_page; page <= end_page; page++) {
        send_data(zero_buffer, SSD1306_WIDTH);
    }
}

void ssd1306_clear_all(void) {
    printf("****** IM HEREEEE\n");

    for (int i = 0; i < 8; i++) {  // 8 pages for 64-pixel height
        ssd1306_clear_lines(0, i);
    }
}

void ssd1306_print_str_at(const char *str, uint8_t page, uint8_t column, bool clear) {
    ssd1306_clear_lines(page, page);
    set_page(page);
    set_column(column); 

    for (int i=0; i<SSD1306_MAX_CHAR; i++) {
        if (*str) {
            uint8_t char_index = *str - 32; // Adjust for ASCII offset
            send_data((uint8_t *)font7x5[char_index], 5); // Send font data
            str++;
        } else {
            // uint8_t empty_data[5] = {0};
            // send_data(empty_data, 5); // Send font data
        }
    }
    // while (*str) {
    //     uint8_t char_index = *str - 32; // Adjust for ASCII offset
    //     send_data((uint8_t *)font7x5[char_index], 5); // Send font data
    //     str++;
    // }
}

void ssd1306_print_str(const char *str, uint8_t page) {
    // ssd1306_print_str_at(str, page, 0, true);
}

static void set_addressing_mode(uint8_t mode) {
    send_cmd(0x20);          // Set addressing mode
    send_cmd(mode & 0x03);   // Mode: 0 = horizontal, 1 = vertical, 2 = page
}

void ssd1306_setup(uint8_t address) {
    ssd1306 = i2c_device_create(I2C_NUM_0, 0x3C);

    // Initialize display
    send_cmd(0xAE); // Display off
    send_cmd(0xD5); // Set display clock divide
    send_cmd(0x80);
    send_cmd(0xA8); // Set multiplex ratio
    send_cmd(0x3F);
    send_cmd(0xD3); // Set display offset
    send_cmd(0x00);
    send_cmd(0x40); // Set start line
    send_cmd(0x8D); // Charge pump
    send_cmd(0x14);
    send_cmd(0x20); // Memory mode
    send_cmd(0x00);
    send_cmd(0xA1); // Segment remap
    send_cmd(0xC8); // COM output scan direction
    send_cmd(0xDA); // COM pins hardware
    send_cmd(0x12);
    send_cmd(0x81); // Contrast control
    send_cmd(0xCF);
    send_cmd(0xD9); // Pre-charge period
    send_cmd(0xF1);
    send_cmd(0xDB); // VCOMH deselect level
    send_cmd(0x30);
    send_cmd(0xA4); // Display resume
    send_cmd(0xA6); // Normal display
    send_cmd(0xAF); // Display on

    ssd1306_clear_all();

    // Page addressing mode
    set_addressing_mode(SSD1306_INITIAL_DISPLAY_MODE);

    // Vertical line - required vertical mode
    // uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
    // send_data(data, sizeof(data));
}


void ssd1306_draw_pixel(uint8_t x, uint8_t y, uint8_t color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return; // Out of bounds

    set_update_target(0, 7);

    // Set the pixel in the buffer
    if (color) {
        buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8)); // Set pixel
    } 
    // else {
    //     buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8)); // Clear pixel
    // }
    send_data(buffer, sizeof(buffer));
}

static uint8_t x_ref = 0;

void ssd1306_push_pixel(uint8_t y, uint8_t color) {
    if (x_ref >= SSD1306_WIDTH) {
        x_ref = 0;
        ssd1306_clear_all();
    }
    ssd1306_draw_pixel(x_ref, y, color);
    x_ref+=4;
}


uint8_t display_buffer[SSD1306_WIDTH * (SSD1306_HEIGHT/8)]; // 1024 bytes

void oled_update() {
    set_update_target(0, 7);
    send_data(display_buffer, sizeof(display_buffer));
}

// Draw vertical bar for spectrum
void draw_bar(uint8_t b, uint8_t height, uint8_t width, uint8_t space) {
    uint16_t x = b * (width + space);
    if (x + width > SSD1306_WIDTH) return;  // Check overflow

    // Precompute masks for all pages
    uint8_t masks[8] = {0};
    uint8_t pages = height / 8;
    uint8_t remainder = height % 8;

    for (uint8_t p = 0; p < 8; p++) {
        if (p < pages) {
            masks[p] = 0xFF;  // Full page is part of the bar
        } else if (p == pages) {
            masks[p] = 0xFF << (8 - remainder);  // Partial page for the remainder
        }
    }

    // Single loop to fill the display_buffer
    for (uint8_t p = 0; p < 8; p++) {
        uint16_t page_offset = (7 - p) * SSD1306_WIDTH + x;
        for (uint8_t col = 0; col < width; col++) {
            display_buffer[page_offset + col] = masks[p];
        }
    }
        
    // printf("b = %u, x = %u\n", b, x);
}

// FFT-based spectrum analyzer core
void spectrum_analyze(uint8_t num_bands) {
    int16_t samples[] = {
        10, 20, 30, 40, 50, 60, 70, 80, 90, 10, 11, 12, 13, 14, 15, 16,
        10, 20, 30, 40, 50, 60, 70, 80, 90, 10, 11, 12, 13, 14, 15, 16
    };

    // Simple energy calculation per band
    uint8_t sample_len = sizeof(samples) / sizeof(samples[0]);
    uint8_t band_heights[num_bands];
    memset(band_heights, 0, sizeof(band_heights));

    for(int i = 0; i < sample_len; i++) {
        int band = map_value(i, 0, sample_len, 0, num_bands);
        band_heights[band] += abs(samples[i]) / 8;
    }

    // Update display
    memset(display_buffer, 0, sizeof(display_buffer));
    for(int b = 0; b < num_bands; b++) {
        uint8_t h = constrain_value(band_heights[b], 0, 63);
        draw_bar(b, h, 5, 1);
    }
    oled_update();
}

#define I2C_MASTER_TIMEOUT_MS 50   // ms
#define I2C_MASTER_NUM I2C_NUM_0

int do_i2cdetect_cmd(uint8_t scl_pin, uint8_t sda_pin) {
    // Initialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_PORT, &conf);
    i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
    
    esp_err_t ret;
    printf("Scanning I2C bus...\n");

    for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
        // Try to read a byte from the device
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 50 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            printf("Device found at address 0x%02X\n", addr);
        }
    }
    printf("scanning done\n");

    return 0;
}