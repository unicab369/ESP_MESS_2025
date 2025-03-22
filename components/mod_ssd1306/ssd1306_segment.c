#include "ssd1306_segment.h"
#include "mod_ssd1306.h"

#define SEGMENT_TOP          (1 << 0)  // Bit 0: Top segment
#define SEGMENT_RIGHT_UPPER  (1 << 1)  // Bit 1: Upper part of the right segment
#define SEGMENT_RIGHT_LOWER  (1 << 2)  // Bit 2: Lower part of the right segment
#define SEGMENT_BOTTOM       (1 << 3)  // Bit 3: Bottom segment
#define SEGMENT_LEFT_UPPER   (1 << 4)  // Bit 4: Upper part of the left segment
#define SEGMENT_LEFT_LOWER   (1 << 5)  // Bit 5: Lower part of the left segment
#define SEGMENT_MIDDLE       (1 << 6)  // Bit 6: Middle segment

// Segment attributes for digits 0-9
const uint8_t DIGIT_ATTRIBUTES[10] = {
    // 0
    SEGMENT_TOP | SEGMENT_LEFT_UPPER | SEGMENT_LEFT_LOWER | 
    SEGMENT_RIGHT_UPPER | SEGMENT_RIGHT_LOWER | SEGMENT_BOTTOM,
    // 1
    SEGMENT_RIGHT_UPPER | SEGMENT_RIGHT_LOWER,
    // 2
    SEGMENT_TOP | SEGMENT_RIGHT_UPPER | SEGMENT_MIDDLE |
    SEGMENT_LEFT_LOWER | SEGMENT_BOTTOM,
    // 3
    SEGMENT_TOP | SEGMENT_RIGHT_UPPER | SEGMENT_RIGHT_LOWER |
    SEGMENT_MIDDLE | SEGMENT_BOTTOM,
    // 4
    SEGMENT_LEFT_UPPER | SEGMENT_RIGHT_UPPER |
    SEGMENT_RIGHT_LOWER | SEGMENT_MIDDLE,
    // 5
    SEGMENT_TOP | SEGMENT_LEFT_UPPER | SEGMENT_MIDDLE |
    SEGMENT_RIGHT_LOWER | SEGMENT_BOTTOM,
    // 6
    SEGMENT_TOP | SEGMENT_LEFT_UPPER | SEGMENT_LEFT_LOWER |
    SEGMENT_MIDDLE | SEGMENT_RIGHT_LOWER | SEGMENT_BOTTOM,
    // 7
    SEGMENT_TOP | SEGMENT_RIGHT_UPPER | SEGMENT_RIGHT_LOWER,
    // 8
    SEGMENT_TOP | SEGMENT_LEFT_UPPER | SEGMENT_LEFT_LOWER |
    SEGMENT_RIGHT_UPPER | SEGMENT_RIGHT_LOWER | SEGMENT_MIDDLE | SEGMENT_BOTTOM,
    // 9
    SEGMENT_TOP | SEGMENT_LEFT_UPPER | SEGMENT_RIGHT_UPPER |
    SEGMENT_RIGHT_LOWER | SEGMENT_MIDDLE | SEGMENT_BOTTOM,
};

static void draw_digit(int8_t digit, int16_t x, int16_t y, uint8_t width, uint8_t height) {
    if (digit > 9) return;

    //! Precompute frequently used values
    int16_t x_start = (x < 0) ? -x : 0;
    int16_t x_end = (x + width > SSD1306_WIDTH) ? SSD1306_WIDTH - x : width;
    int16_t y_start = (y < 0) ? -y : 0;
    int16_t y_end = (y + height > SSD1306_HEIGHT) ? SSD1306_HEIGHT - y : height;

    //! Draw digit #1 in the middle of digit frame
    if (digit == 1) {
        //! Calculate the middle position for the digit '1'
        int16_t middle_x = x + (width / 2); // Center of the bounding box

        //! Precompute page and bitmask for vertical segments
        for (int16_t dy = y_start; dy < y_end; dy++) {
            int16_t pixel_y = y + dy;
            uint8_t page = page_masks[pixel_y].page;
            uint8_t bitmask = page_masks[pixel_y].bitmask;
            frame_buffer[page][middle_x] |= bitmask;
        }
        return;
    }

    //! Precompute page and bitmask for horizontal segments
    uint8_t page_top = page_masks[y + y_start].page;
    uint8_t bitmask_top = page_masks[y + y_start].bitmask;
    uint8_t page_mid = page_masks[y + height / 2].page;
    uint8_t bitmask_mid = page_masks[y + height / 2].bitmask;
    uint8_t page_bottom = page_masks[y + y_end - 1].page;
    uint8_t bitmask_bottom = page_masks[y + y_end - 1].bitmask;

    //! Get the segment attributes for the digit
    uint8_t attributes = DIGIT_ATTRIBUTES[digit];

    //! Draw all horizontal segments at once
    if (attributes & (SEGMENT_TOP | SEGMENT_MIDDLE | SEGMENT_BOTTOM)) {
        for (int16_t dx = x_start; dx < x_end; dx++) {
            int16_t pixel_x = x + dx;

            if (attributes & SEGMENT_TOP) {
                frame_buffer[page_top][pixel_x] |= bitmask_top;
            }
            if (attributes & SEGMENT_MIDDLE) {
                frame_buffer[page_mid][pixel_x] |= bitmask_mid;
            }
            if (attributes & SEGMENT_BOTTOM) {
                frame_buffer[page_bottom][pixel_x] |= bitmask_bottom;
            }
        }
    }

    //! Draw all vertical segments at once
    if (attributes & (SEGMENT_LEFT_UPPER | SEGMENT_LEFT_LOWER | SEGMENT_RIGHT_UPPER | SEGMENT_RIGHT_LOWER)) {
        for (int16_t dy = y_start; dy < y_end; dy++) {
            int16_t pixel_y = y + dy;
            uint8_t page = page_masks[pixel_y].page;
            uint8_t bitmask = page_masks[pixel_y].bitmask;

            if ((dy < height / 2 && (attributes & SEGMENT_LEFT_UPPER)) || // Left upper
                (dy >= height / 2 && (attributes & SEGMENT_LEFT_LOWER))) { // Left lower
                frame_buffer[page][x + x_start] |= bitmask;
            }

            if ((dy < height / 2 && (attributes & SEGMENT_RIGHT_UPPER)) || // Right upper
                (dy >= height / 2 && (attributes & SEGMENT_RIGHT_LOWER))) { // Right lower
                frame_buffer[page][x + x_end - 1] |= bitmask;
            }
        }
    }
}

void ssd1306_print_digits(const char *str, int8_t x, int8_t y, uint8_t width, uint8_t height, int8_t space) {
    size_t length = strlen(str);        // Calculate the length of the string

    for (size_t i = 0; i < length; i++) {
        int8_t number = (int8_t)(str[i] - '0');         // Convert char to int8_t
        draw_digit(number, x, y, width, height);
        x += width + space;                             // Update the x position for the next digit
    }
}

void ssd1306_test_digits() {
    if (ssd1306_print_mode != 0) return;

    //! Clear the buffer
    memset(frame_buffer, 0, sizeof(frame_buffer));

    precompute_page_masks();

    ssd1306_print_digits("0123456789", 0, 0, 5, 7, 2);
    ssd1306_print_digits("0123456789", 0, 10, 7, 9, 2);
    ssd1306_print_digits("0123456789", 0, 23, 9, 11, 2);
    ssd1306_print_digits("0123456789", 0, 40, 10, 13, 2);

    ssd1306_update_frame();
}