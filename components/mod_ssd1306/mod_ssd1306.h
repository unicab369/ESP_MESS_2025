#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

#define SSD1306_PAGES       (SSD1306_HEIGHT / 8) // 8 pages for 64 rows
#define MAX_PAGE_INDEX      (SSD1306_PAGES - 1)

#define SSD1306_MAX_WIDTH_INDEX     (SSD1306_WIDTH - 1)
#define SSD1306_MAX_HEIGHT_INDEX    (SSD1306_HEIGHT - 1)

void ssd1306_setup(uint8_t address);
void ssd1306_set_addressing_mode(uint8_t mode);
void ssd1306_set_update_target(uint8_t start_page, uint8_t end_page);

void ssd1306_set_column_address(uint8_t start_col, uint8_t end_col);
void ssd1306_set_page_address(uint8_t start_page, uint8_t end_page);

void ssd1306_send_cmd(uint8_t value);
void ssd1306_send_data(const uint8_t *data, size_t len);

void ssd1306_print_str(const char *str, uint8_t line);
void ssd1306_print_str_at(const char *str, uint8_t page, uint8_t column, bool clear);
void ssd1306_clear_all(void);


int do_i2cdetect_cmd(uint8_t scl_pin, uint8_t sda_pin);

void ssd1306_clear_frameBuffer();
void ssd1306_update_frame();

extern int8_t ssd1306_print_mode;

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