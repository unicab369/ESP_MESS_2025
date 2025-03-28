#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mod_i2c.h"

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

#define SSD1306_PAGES       (SSD1306_HEIGHT / 8) // 8 pages for 64 rows
#define MAX_PAGE_INDEX      (SSD1306_PAGES - 1)

#define SSD1306_MAX_WIDTH_INDEX     (SSD1306_WIDTH - 1)
#define SSD1306_MAX_HEIGHT_INDEX    (SSD1306_HEIGHT - 1)

void ssd1306_setup(M_I2C_Device *device);
void ssd1306_print_str(M_I2C_Device *device, const char *str, uint8_t page);

void ssd1306_print_str_at(
    M_I2C_Device *device, const char *str,
    uint8_t page, uint8_t column, bool clear
);

void ssd1306_clear_all(M_I2C_Device *device);

void ssd1306_clear_frameBuffer();
void ssd1306_update_frame(M_I2C_Device *device);


typedef struct {
    uint8_t pos;         // X or y position of the line
    uint8_t start;      // Start position of the line
    uint8_t end;        // End position of the line
} M_Line;

typedef struct {
    uint8_t page;
    uint8_t bitmask;
} M_Page_Mask;


extern uint8_t frame_buffer[SSD1306_PAGES][SSD1306_WIDTH];
extern M_Page_Mask page_masks[SSD1306_HEIGHT];

void precompute_page_masks();
void ssd1306_set_printMode(uint8_t direction);
