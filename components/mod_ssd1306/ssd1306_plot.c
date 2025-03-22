#include "ssd1306_plot.h"

#include "mod_ssd1306.h"
#include <stdio.h>
#include <string.h>
#include <mod_utility.h>

void ssd1306_horizontal_line(const M_Line *line, uint8_t thickness, uint8_t flipped) {
    uint8_t start_y = line->pos;
    uint8_t end_y = start_y + thickness - 1; // Calculate the end y-coordinate

    //! Clamp end_y to avoid exceeding display height
    if (end_y >= SSD1306_HEIGHT) end_y = SSD1306_HEIGHT - 1;
    uint8_t x = line->start;
    uint8_t end_x = line->end;

    if (flipped) {
        for (uint8_t y = start_y; y <= end_y; y++) {
            uint8_t page = page_masks[y].page;
            uint8_t bitmask = page_masks[y].bitmask;

            uint8_t current_x = SSD1306_WIDTH - 1 - x;
            uint8_t current_end_x = SSD1306_WIDTH - 1 - end_x;

            //! Ensure current_x <= current_end_x
            if (current_x > current_end_x) {
                uint8_t temp = current_x;
                current_x = current_end_x;
                current_end_x = temp;
            }

            uint16_t bulk_bitmask = ((uint16_t)bitmask << 8) | ((uint16_t)bitmask);
            uint8_t *row_ptr = &frame_buffer[page][current_x];

            //! Optimization: Process 2 bytes at a time using uint16_t
            while (current_x + 2 <= current_end_x) {
                *((uint16_t *)row_ptr) |= bulk_bitmask;
                row_ptr += 2;
                current_x += 2;
            }

            //! Process remaining byte (if any)
            if (current_x <= current_end_x) {
                *row_ptr++ |= bitmask;
                current_x++;
            }
        }
    } else {
        for (uint8_t y = start_y; y <= end_y; y++) {
            uint8_t page = page_masks[y].page;
            uint8_t bitmask = page_masks[y].bitmask;
    
            uint16_t bulk_bitmask = ((uint16_t)bitmask << 8) | ((uint16_t)bitmask);
            uint8_t *row_ptr = &frame_buffer[page][x];

            //! Optimization: Process 2 bytes at a time using uint16_t
            while (x + 2 <= end_x) {
                *((uint16_t *)row_ptr) |= bulk_bitmask;
                row_ptr += 2;
                x += 2;
            }

            //! Process remaining byte
            if (x <= end_x) {
                *row_ptr++ |= bitmask;
                x++;
            }

            //! Reset x for the next y-coordinate
            x = line->start;
        }
    }
}

void ssd1306_vertical_line(const M_Line *line, uint8_t thickness, uint8_t flipped) {
    uint8_t start_y = flipped ? (SSD1306_HEIGHT - 1 - line->end) : line->start;
    uint8_t end_y = flipped ? (SSD1306_HEIGHT - 1 - line->start) : line->end;

    uint8_t start_page = page_masks[start_y].page;
    uint8_t end_page = page_masks[end_y].page;

    //! Iterate through the width of the line
    for (uint8_t w = 0; w < thickness; w++) {
        uint8_t x_value = line->pos + w;

        //! Optimization: drawing segments by page
        //! Handle full pages: pages between start_page and end_page are full
        for (uint8_t page = start_page + 1; page < end_page; page++) {
            frame_buffer[page][x_value] |= 0xFF; // Set all bits in the page
        }

        //! Handle the start page
        uint8_t bitmask = 0;
        if (start_y % 8 == 0) {
            // Start page is full
            bitmask |= 0xFF;
        } else {
            // Start page is partial
            for (uint8_t y = start_y; y <= end_y && page_masks[y].page == start_page; y++) {
                bitmask |= page_masks[y].bitmask;
            }
        }
        frame_buffer[start_page][x_value] |= bitmask;

        //! Handle the end page
        if (end_page != start_page) {
            bitmask = 0;
            if ((end_y + 1) % 8 == 0) {
                // End page is full
                bitmask |= 0xFF;
            } else {
                // End page is partial
                for (uint8_t y = start_y; y <= end_y && page_masks[y].page == end_page; y++) {
                    bitmask |= page_masks[y].bitmask;
                }
            }
            frame_buffer[end_page][x_value] |= bitmask;
        }
    }
}

void ssd1306_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t thickness) {
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;      // Error term
    int16_t e2;                 // Temporary error term
    uint8_t flipped = false;

    // Iterate until the line is fully drawn
    while (1) {
        // Draw a pixel at (x0, y0) with width
        for (uint8_t t = 0; t < thickness; t++) {
            int16_t current_y = y0 + t;
            if (current_y >= 0 && current_y < SSD1306_HEIGHT) {
                uint8_t page = page_masks[current_y].page;
                uint8_t bitmask = page_masks[current_y].bitmask;

                // Calculate the x-coordinate (mirrored if needed)
                int16_t current_x = flipped ? (SSD1306_MAX_WIDTH_INDEX - x0) : x0;

                // Ensure the x-coordinate is within bounds
                if (current_x >= 0 && current_x < SSD1306_WIDTH) {
                    frame_buffer[page][current_x] |= bitmask;
                }
            }
        }

        //! Check if the line is complete
        if (x0 == x1 && y0 == y1) break;

        //! Update the error term
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx; // Move in x-direction
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy; // Move in y-direction
        }
    }
}

void ssd1306_rectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    uint8_t max_x = x + width;
    uint8_t max_y = y + height;

    M_Line hor_line0 = { y,     x, max_x };
    M_Line hor_line1 = { max_y, x, max_x };
    ssd1306_horizontal_line(&hor_line0, 1, false);
    ssd1306_horizontal_line(&hor_line1, 1, false);

    M_Line ver_line0 = { x,     y, max_y };
    M_Line ver_line1 = { max_x, y, max_y };
    ssd1306_vertical_line(&ver_line0, 1, false);
    ssd1306_vertical_line(&ver_line1, 1, false);
}

void ssd1306_triangle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    ssd1306_draw_line(x0, y0, x1, y1, 1);
    ssd1306_draw_line(x1, y1, x2, y2, 1);
    ssd1306_draw_line(x2, y2, x0, y0, 1);
}

#define MASK_HEIGHT 7
#define MASK_WIDTH 5

#define SEGMENT_TOP          (1 << 0)  // Bit 0: Top segment
#define SEGMENT_RIGHT_UPPER  (1 << 1)  // Bit 1: Upper part of the right segment
#define SEGMENT_RIGHT_LOWER  (1 << 2)  // Bit 2: Lower part of the right segment
#define SEGMENT_BOTTOM       (1 << 3)  // Bit 3: Bottom segment
#define SEGMENT_LEFT_UPPER   (1 << 4)  // Bit 4: Upper part of the left segment
#define SEGMENT_LEFT_LOWER   (1 << 5)  // Bit 5: Lower part of the left segment
#define SEGMENT_MIDDLE       (1 << 6)  // Bit 6: Middle segment

// Combined attributes
#define SEGMENT_RIGHT_BOTH   (SEGMENT_RIGHT_UPPER | SEGMENT_RIGHT_LOWER) // Both right segments
#define SEGMENT_LEFT_BOTH    (SEGMENT_LEFT_UPPER | SEGMENT_LEFT_LOWER)   // Both left segments

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

const uint8_t LINE_MASK[MASK_HEIGHT] = {
    0b1111111, // Row 0: Top line (all pixels set)
    0b1000001, // Row 1: Left and right lines (first and last pixels set)
    0b1000001, // Row 2: Left and right lines
    0b1111111, // Row 3: Middle line (all pixels set)
    0b1000001, // Row 4: Left and right lines
    0b1000001, // Row 5: Left and right lines
    0b1111111, // Row 6: Bottom line (all pixels set)
};

void draw_digit(uint8_t digit, int16_t x, int16_t y) {
    if (digit > 9) return;
    if (x + MASK_WIDTH <= 0 || x >= SSD1306_WIDTH ||
        y + MASK_HEIGHT <= 0 || y >= SSD1306_HEIGHT) return;

    // Get the segment attributes for the digit
    uint8_t attributes = DIGIT_ATTRIBUTES[digit];

    //! Precompute frequently used values
    int16_t x_end = x + MASK_WIDTH - 1;
    int16_t y_end = y + MASK_HEIGHT - 1;
    int16_t y_mid = y + MASK_HEIGHT / 2;

    //! Batch update all segments in a single nested loop
    for (int16_t dy = y; dy <= y_end; dy++) {
        if (dy < 0 || dy >= SSD1306_HEIGHT) continue;

        uint8_t page = page_masks[dy].page;
        uint8_t bitmask = page_masks[dy].bitmask;

        for (int16_t dx = x; dx <= x_end; dx++) {
            if (dx < 0 || dx >= SSD1306_WIDTH) continue;

            // Check if the current pixel belongs to any segment
            bool is_horizontal = (dy == y && (attributes & SEGMENT_TOP)) ||      // Top segment
                                (dy == y_mid && (attributes & SEGMENT_MIDDLE)) || // Middle segment
                                (dy == y_end && (attributes & SEGMENT_BOTTOM));  // Bottom segment

            bool is_vertical = (dx == x && ((dy < y_mid && (attributes & SEGMENT_LEFT_UPPER)) || // Left upper
                                           (dy >= y_mid && (attributes & SEGMENT_LEFT_LOWER)))) || // Left lower
                              (dx == x_end && ((dy < y_mid && (attributes & SEGMENT_RIGHT_UPPER)) || // Right upper
                                              (dy >= y_mid && (attributes & SEGMENT_RIGHT_LOWER)))); // Right lower

            // Update the frame buffer if the pixel belongs to any segment
            if (is_horizontal || is_vertical) {
                frame_buffer[page][dx] |= bitmask;
            }
        }
    }
}

void ssd1306_spectrum(uint8_t num_band)  {
    if (ssd1306_print_mode != 0) return;
    precompute_page_masks();

    draw_digit(1, 10, 10);
    draw_digit(2, 20, 10);
    draw_digit(3, 30, 10);
    draw_digit(4, 40, 10);
    draw_digit(5, 50, 10);
    draw_digit(6, 60, 10);
    draw_digit(7, 70, 10);
    draw_digit(8, 80, 10);
    draw_digit(9, 90, 10);

    ssd1306_update_frame();
}


// int16_t samples[] = {
//     10, 20, 30, 40, 50, 60, 35, 20, 63, 10, 11, 12, 13, 63, 15, 16,
//     10, 20, 30, 63, 50, 60, 63, 63, 63, 10, 63, 12, 13, 14, 15, 16
// };

// // FFT-based spectrum analyzer core
// void ssd1306_spectrum(uint8_t num_band) {
//     // Simple energy calculation per band
//     M_Line bands[num_band];
//     memset(bands, 0, sizeof(bands));

//     uint16_t sample_len = sizeof(samples) / sizeof(samples[0]);
//     uint16_t samp_per_band = sample_len / num_band;

//     for (int i = 0; i < num_band; i++) {
//         int start_index = i * samp_per_band;
//         int end_index = start_index + samp_per_band;

//         uint16_t sum = 0;
//         for (int i = start_index; i < end_index; i++) {
//             sum += abs(samples[i]); // Use absolute value as magnitude
//         }

//         // Calculate the average magnitude for the bin
//         bands[i].pos = i * 5;
//         bands[i].end = sum / samp_per_band;
//     }

//     //! Clear the buffer
//     memset(frame_buffer, 0, sizeof(frame_buffer));

//      //! precompute page masks
//     precompute_page_masks();

//     //! Draw bars
//     for (int i = 0; i < num_band; i++) {
//         ssd1306_vertical_line(&bands[i], 4, true);
//         ssd1306_horizontal_line(&bands[i], 4, true);
//     }

//     //! Draw rectangle
//     ssd1306_rectangle(30, 10, 70, 30);

//     //! Draw line
//     ssd1306_draw_line(20, 20, 80, 50, 3);

//     //! Draw triangle
//     ssd1306_triangle(55, 25, 90, 20, 70, 60);

//     //! Update the display
//     ssd1306_update_frame();
// }
