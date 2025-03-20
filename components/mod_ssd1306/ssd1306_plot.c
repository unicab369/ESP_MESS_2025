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

// FFT-based spectrum analyzer core
void ssd1306_spectrum(uint8_t band_cnt) {
    int16_t samples[] = {
        10, 20, 30, 40, 50, 60, 70, 80, 150, 10, 11, 12, 13, 100, 15, 16,
        10, 20, 30, 100, 50, 60, 150, 80, 90, 10, 100, 12, 13, 14, 15, 16
    };

    // Simple energy calculation per band
    uint8_t sample_len = sizeof(samples) / sizeof(samples[0]);
    uint8_t band_heights[band_cnt];
    memset(band_heights, 0, sizeof(band_heights));

    for (int i = 0; i < sample_len; i++) {
        int band = map_value(i, 0, sample_len, 0, band_cnt);
        band_heights[band] += abs(samples[i]) >> 3;  // Equivalent to / 8
    }

    // Constrain band heights to the display height
    for (int b = 0; b < band_cnt; b++) {
        band_heights[b] = constrain_value(band_heights[b], 0, 63);
    }

    //! horizontal addressing mode
    ssd1306_set_addressing_mode(0x00);

    //! Precompute bar masks
    precompute_bar_masks(SSD1306_HEIGHT, true);

    //! Clear the frame buffer
    memset(frame_buffer, 0, sizeof(frame_buffer));

    //! Draw all bars
    for (int b = 0; b < band_cnt; b++) {
        draw_bar_vertical_mode(b, band_heights[b], 5, 1);
    }

    //! Update the display
    ssd1306_update_frame();
}

// Draw a vertical line in the frame buffer
void ssd1306_draw_vertical_line(uint8_t x, uint8_t start_y, uint8_t end_y) {
    for (uint8_t y = start_y; y <= end_y; y++) {
        uint8_t page = y / 8;           // Calculate the page
        uint8_t bit_mask = 1 << (y % 8); // Calculate the bit mask
        frame_buffer[page][x] |= bit_mask; // Set the pixel at (x, y)
    }
}

// void ssd1306_spectrum(uint8_t band_cnt) {
//     //! vertical addressing mode
//     ssd1306_set_addressing_mode(0x01);

//     //! Clear the frame buffer
//     memset(frame_buffer, 0, sizeof(frame_buffer));

//     // Draw multiple vertical lines
//     ssd1306_draw_vertical_line(10, 5, 30);  // Line from (10, 5) to (10, 30)
//     ssd1306_draw_vertical_line(50, 10, 40); // Line from (50, 10) to (50, 40)
//     ssd1306_draw_vertical_line(100, 20, 60);// Line from (100, 20) to (100, 60)

//     //! Update the display
//     ssd1306_update_frame();
// }