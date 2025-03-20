#include "ssd1306_plot.h"

#include "mod_ssd1306.h"
#include <stdio.h>
#include <string.h>

#include <mod_utility.h>

// The bar_masks array is a lookup table that stores precomputed patterns (masks) for drawing
// vertical bars of different heights on the SSD1306 display. Each entry in bar_masks corresponds to
// a specific height of a bar and contains the bit patterns needed to draw that bar on the display.

// bar_masks is a 2D array: bar_masks[height][page].
// - height: The height of the bar (0 to 63 pixels).
// - page: The page (0 to 7) within the bar.
// Each entry bar_masks[height][page] contains the bit pattern (mask) for that page of the bar.
uint8_t bar_masks[SSD1306_HEIGHT][SSD1306_PAGES];  // Lookup table for bar masks
uint8_t frame_buffer[SSD1306_PAGES][SSD1306_WIDTH];
uint8_t column_buffer[SSD1306_WIDTH][SSD1306_PAGES];

//! Precompute bar_masks
void precompute_bar_masks(uint8_t height, bool flipped) {
    for (uint8_t h = 0; h < 64; h++) {
        uint8_t pages = h >> 3;             // Equivalent to height / 8
        uint8_t remainder = h & 0x07;       // Equivalent to height % 8

        for (uint8_t p = 0; p < 8; p++) {
            uint8_t page_idx = flipped ? 7 - p : p;

            if (p < pages) {
                bar_masks[h][page_idx] = 0xFF;  // Full page
            } else if (p == pages) {
                bar_masks[h][page_idx] = 0xFF << (8 - remainder);  // Partial page
            } else {
                bar_masks[h][page_idx] = 0x00;  // Empty page
            }
        }
    }
}

// Draw vertical bar for spectrum
void draw_bar_vertical_mode(uint8_t b, uint8_t height, uint8_t width, uint8_t space) {
    uint16_t x = b * (width + space);  // Starting x position of the bar
    if (x + width > SSD1306_WIDTH) return;  // Check overflow

    // Draw the bar
    for (uint8_t col = 0; col < width; col++) {
        for (uint8_t p = 0; p < SSD1306_PAGES; p++) {
            frame_buffer[p][x + col] = bar_masks[height][p];
        }
    }
}

// // FFT-based spectrum analyzer core
// void ssd1306_spectrum(uint8_t band_cnt) {
//     int16_t samples[] = {
//         10, 20, 30, 40, 50, 60, 70, 80, 150, 10, 11, 12, 13, 100, 15, 16,
//         10, 20, 30, 100, 50, 60, 150, 80, 90, 10, 100, 12, 13, 14, 15, 16
//     };

//     // Simple energy calculation per band
//     uint8_t sample_len = sizeof(samples) / sizeof(samples[0]);
//     uint8_t band_heights[band_cnt];
//     memset(band_heights, 0, sizeof(band_heights));

//     for (int i = 0; i < sample_len; i++) {
//         int band = map_value(i, 0, sample_len, 0, band_cnt);
//         band_heights[band] += abs(samples[i]) >> 3;  // Equivalent to / 8
//     }

//     // Constrain band heights to the display height
//     for (int b = 0; b < band_cnt; b++) {
//         band_heights[b] = constrain_value(band_heights[b], 0, 63);
//     }

//     //! horizontal addressing mode
//     ssd1306_set_addressing_mode(0x00);

//     //! Precompute bar masks
//     precompute_bar_masks(SSD1306_HEIGHT, true);

//     //! Clear the frame buffer
//     memset(frame_buffer, 0, sizeof(frame_buffer));

//     //! Draw all bars
//     for (int b = 0; b < band_cnt; b++) {
//         draw_bar_vertical_mode(b, band_heights[b], 5, 1);
//     }

//     //! Update the display
//     ssd1306_update_frame();
// }


typedef struct {
    uint8_t page;
    uint8_t bitmask;
} M_Page_Vertical_BitMask;

M_Page_Vertical_BitMask page_vertical_bitmasks[SSD1306_HEIGHT];

typedef struct {
    uint8_t x;
    uint8_t start_y;
    uint8_t end_y;
} M_VerticalLine;

void make_page_vertical_bitmasks() {
    for (uint8_t y = 0; y < SSD1306_HEIGHT; y++) {
        page_vertical_bitmasks[y].page = y >> 3;                    // y / 8
        page_vertical_bitmasks[y].bitmask = (1 << (y & 0x07));      // y % 8
    }
}

void ssd1306_draw_vertical_lines(const M_VerticalLine *lines, uint8_t num_lines) {
    //! Iterate over all vertical pixels at once
    for (uint8_t y = 0; y < SSD1306_HEIGHT; y++) {
        //! Fetch precomputed page and bit mask
        uint8_t page = page_vertical_bitmasks[y].page;
        uint8_t bitmask = page_vertical_bitmasks[y].bitmask;

        //! Iterate through all requested vertical lines
        for (uint8_t i = 0; i < num_lines; i++) {
            if (y < lines[i].start_y || y > lines[i].end_y) continue;
            frame_buffer[page][lines[i].x] |= bitmask; // Set the bit in the frame buffer
        }
    }

    ssd1306_update_frame();
}

void ssd1306_spectrum(uint8_t band_cnt) {

    M_VerticalLine lines[] = {
        {10, 5, 30},
        {50, 10, 40},
        {100, 20, 60}
    };
    uint8_t num_lines = sizeof(lines) / sizeof(lines[0]);

    make_page_vertical_bitmasks();
    ssd1306_draw_vertical_lines(lines, num_lines);
}


//////////////////////

// uint8_t column_buffer[SSD1306_WIDTH][SSD1306_PAGES];

// void ssd1306_draw_vertical_line(uint8_t col, uint8_t y0, uint8_t y1) {
//     // Ensure y0 <= y1
//     if (y0 > y1) {
//         uint8_t temp = y0;
//         y0 = y1;
//         y1 = temp;
//     }

//     uint8_t start_page = y0 >> 3;       // y0 / 8
//     uint8_t end_page = y1 >> 3;         // y1 / 8
//     uint8_t start_bit = y0 & 0x07;      // y0 % 8
//     uint8_t end_bit = y1 & 0x07;        // y1 % 8

//     // Precompute masks for the start and end pages
//     uint8_t start_mask = 0xFF << start_bit;
//     uint8_t end_mask = ~(0xFF << (end_bit + 1));

//     // Update the column buffer
//     for (uint8_t p = start_page; p <= end_page; p++) {
//         uint8_t mask = 0xFF;
//         if (p == start_page) mask &= start_mask;
//         if (p == end_page) mask &= end_mask;
//         column_buffer[col][p] |= mask;  // Use OR to preserve existing data
//     }
// }

// void ssd1306_update_display() {
//     // Set addressing mode to vertical
//     ssd1306_set_addressing_mode(1);

//     // Set the column and page address to cover the entire display
//     ssd1306_set_column_address(0, SSD1306_WIDTH - 1);
//     ssd1306_set_page_address(0, SSD1306_PAGES - 1);

//     // Send the entire frame buffer to the display
//     ssd1306_send_data((uint8_t *)column_buffer, sizeof(column_buffer));
// }

// void ssd1306_spectrum(uint8_t band_cnt) {
//     // Clear the frame buffer
//     memset(column_buffer, 0, sizeof(column_buffer));

//     // Draw multiple vertical lines
//     ssd1306_draw_vertical_line(0, 10, 40);   // Vertical line at x = 0, from y = 10 to y = 50
//     ssd1306_draw_vertical_line(32, 30, 35);   // Vertical line at x = 32, from y = 5 to y = 60
//     ssd1306_draw_vertical_line(64, 20, 30);  // Vertical line at x = 64, from y = 20 to y = 40
//     ssd1306_draw_vertical_line(96, 30, 55);   // Vertical line at x = 96, from y = 0 to y = 63

//     // Update the display with the frame buffer
//     ssd1306_update_display();
// }
