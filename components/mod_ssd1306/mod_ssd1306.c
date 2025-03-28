#include "mod_ssd1306.h"

#include "mod_utility.h"
#include "mod_bitmap.h"

#define SSD1306_MAX_CHAR (SSD1306_WIDTH / 5)
// #define SSD1306_MAX_PRINTMODE 4

// static const char *TAG = "SSD1306";
static uint8_t zero_buffer[SSD1306_WIDTH] = {0}; // Buffer of zeros (128 bytes)
// static M_I2C_Device *ssd1306 = NULL;

uint8_t frame_buffer[SSD1306_PAGES][SSD1306_WIDTH] = {0};
M_Page_Mask page_masks[SSD1306_HEIGHT];

void precompute_page_masks() {
    for (uint8_t y = 0; y < SSD1306_HEIGHT; y++) {
        page_masks[y].page       = y >> 3;             // (y / 8)
        page_masks[y].bitmask    = 1 << (y & 0x07);    // (y % 8)
    }
}

// Write command to SSD1306
static void ssd1306_send_cmd( M_I2C_Device *device, uint8_t value) {
    uint8_t buff[2] = {0x00, value};                    // CMD Register 0x00
    i2c_write_register_byte(device, 0x00, value);
}

// Write data to SSD1306
static void ssd1306_send_data( M_I2C_Device *device, const uint8_t *data, size_t len) {
    i2c_write_register(device, 0x40, data, len);       // DATA Register 0x40
}

static void ssd1306_set_addressing_mode(M_I2C_Device *device, uint8_t mode) {
    ssd1306_send_cmd(device, 0x20);          // Set addressing mode
    ssd1306_send_cmd(device, mode & 0x03);   // Mode: 0 = horizontal, 1 = vertical, 2 = page
}

static void ssd1306_set_column_address(M_I2C_Device *device, uint8_t start_col, uint8_t end_col) {
    ssd1306_send_cmd(device, 0x21);         // Set column address
    ssd1306_send_cmd(device, start_col);    // Start column
    ssd1306_send_cmd(device, end_col);      // End column
}

static void ssd1306_set_page_address(M_I2C_Device *device, uint8_t start_page, uint8_t end_page) {
    ssd1306_send_cmd(device, 0x22);         // Set page address
    ssd1306_send_cmd(device, start_page);   // Start page
    ssd1306_send_cmd(device, end_page);     // End page
}

void ssd1306_clear_frameBuffer() {
    // Clear the entire frame buffer (set all pixels to OFF)
    memset(frame_buffer, 0x00, sizeof(frame_buffer));
}

void ssd1306_update_frame(M_I2C_Device *device) {
    ssd1306_set_column_address(device, 0, SSD1306_MAX_WIDTH_INDEX);
    ssd1306_set_page_address(device, 0, MAX_PAGE_INDEX);
    ssd1306_send_data(device, (uint8_t *)frame_buffer, sizeof(frame_buffer));
}

void ssd1306_clear_lines(M_I2C_Device *device, uint8_t start_page, uint8_t end_page) {
    if (end_page>MAX_PAGE_INDEX) return;
    ssd1306_set_column_address(device, 0, SSD1306_MAX_WIDTH_INDEX);
    ssd1306_set_page_address(device, start_page, end_page);

    // Send zero buffer for each page
    for (uint8_t page = start_page; page <= end_page; page++) {
        ssd1306_send_data(device, zero_buffer, SSD1306_WIDTH);
    }
}

void ssd1306_clear_all(M_I2C_Device *device) {
    for (int i = 0; i < 8; i++) {  // 8 pages for 64-pixel height
        ssd1306_clear_lines(device, 0, i);
    }
}

void ssd1306_print_str_at(
    M_I2C_Device *device, const char *str,
    uint8_t page, uint8_t column, bool clear
) {
    ssd1306_clear_lines(device, page, page);

    for (int i=0; i<SSD1306_MAX_CHAR; i++) {
        if (*str) {
            uint8_t char_index = *str - 32; // Adjust for ASCII offset
            ssd1306_send_data(device, (uint8_t *)FONT_7x5[char_index], 5); // Send font data
            str++;
        } else {
            // uint8_t empty_data[5] = {0};
            // ssd1306_send_data(empty_data, 5); // Send font data
        }
    }
    // while (*str) {
    //     uint8_t char_index = *str - 32; // Adjust for ASCII offset
    //     ssd1306_send_data((uint8_t *)font7x5[char_index], 5); // Send font data
    //     str++;
    // }
}

void ssd1306_print_str(M_I2C_Device *display, const char *str, uint8_t page) {
    ssd1306_print_str_at(display, str, page, 0, true);
}

void ssd1306_setup(M_I2C_Device *device) {
    // Initialize display
    ssd1306_send_cmd(device, 0xAE); // Display off
    ssd1306_send_cmd(device, 0xD5); // Set display clock divide
    ssd1306_send_cmd(device, 0x80);
    ssd1306_send_cmd(device, 0xA8); // Set multiplex ratio
    ssd1306_send_cmd(device, 0x3F);
    ssd1306_send_cmd(device, 0xD3); // Set display offset
    ssd1306_send_cmd(device, 0x00);
    ssd1306_send_cmd(device, 0x40); // Set start line
    ssd1306_send_cmd(device, 0x8D); // Charge pump
    ssd1306_send_cmd(device, 0x14);
    ssd1306_send_cmd(device, 0x20); // Memory mode
    ssd1306_send_cmd(device, 0x00);
    ssd1306_send_cmd(device, 0xA1); // Segment remap
    ssd1306_send_cmd(device, 0xC8); // COM output scan direction
    ssd1306_send_cmd(device, 0xDA); // COM pins hardware
    ssd1306_send_cmd(device, 0x12);
    ssd1306_send_cmd(device, 0x81); // Contrast control
    ssd1306_send_cmd(device, 0xCF);
    ssd1306_send_cmd(device, 0xD9); // Pre-charge period
    ssd1306_send_cmd(device, 0xF1);
    ssd1306_send_cmd(device, 0xDB); // VCOMH deselect level
    ssd1306_send_cmd(device, 0x30);
    ssd1306_send_cmd(device, 0xA4); // Display resume
    ssd1306_send_cmd(device, 0xA6); // Normal display
    ssd1306_send_cmd(device, 0xAF); // Display on

    ssd1306_clear_all(device);

    // Page addressing mode

    // 0x00 - Horizontal: Column pointer increments, wraps to next page after reaching end of line.
    // 0x01 - Vertical: Page pointer increments, wraps to next column after reaching end of page.
    // 0x02 - Page: Column pointer increments, stops after reaching end of page.
    ssd1306_set_addressing_mode(device, 0x00);

    // Vertical line - required vertical mode
    // uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
    // ssd1306_send_data(data, sizeof(data));
}