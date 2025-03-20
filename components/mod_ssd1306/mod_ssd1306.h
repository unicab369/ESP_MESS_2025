#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_PAGES (SSD1306_HEIGHT / 8)  // 8 pages for 64 rows

// Declare the frame_buffer as a 2D array
extern uint8_t frame_buffer[SSD1306_PAGES][SSD1306_WIDTH];


void ssd1306_setup(uint8_t address);
void ssd1306_set_addressing_mode(uint8_t mode);
void ssd1306_send_cmd(uint8_t value);
void ssd1306_send_data(const uint8_t *data, size_t len);

void ssd1306_print_str(const char *str, uint8_t line);
void ssd1306_print_str_at(const char *str, uint8_t page, uint8_t column, bool clear);
void ssd1306_clear_all(void);


int do_i2cdetect_cmd(uint8_t scl_pin, uint8_t sda_pin);

void ssd1306_update_frame();