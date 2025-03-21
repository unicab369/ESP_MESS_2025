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
} M_Page_Mask;

M_Page_Mask page_masks[SSD1306_HEIGHT];

typedef struct {
    uint8_t xy;         // X or y position of the line
    uint8_t start;      // Start position of the line
    uint8_t end;        // End position of the line
} M_Line;


void precompute_page_masks(uint8_t inverted) {
    for (uint8_t y = 0; y < SSD1306_HEIGHT; y++) {
        uint8_t y_value = inverted ? SSD1306_HEIGHT - 1 - y : y;
        page_masks[y].page       = y_value >> 3;             // (y_value / 8)
        page_masks[y].bitmask    = 1 << (y_value & 0x07);    // (y_value % 8)
    }
}

// Draw vertical lines
void ssd1306_vertical_lines(const M_Line *lines, uint8_t num_lines) {
    //! Iterate through all requested vertical lines
    for (uint8_t i = 0; i < num_lines; i++) {
        M_Line line = lines[i];
        
        if (line.start == 0) {
            //! Use a pointer to the start of the column in the frame buffer
            uint8_t *col_ptr = &frame_buffer[0][line.xy];

            //! Iterate through the y range of the current line
            for (uint8_t y = 0; y <= line.end; y++) {
                //! Fetch precomputed page and bitmask for y
                uint8_t page = page_masks[y].page;
                uint8_t bitmask = page_masks[y].bitmask;
                col_ptr[page * SSD1306_WIDTH] |= bitmask;   // set the bits in frame_buffer
            }
        }
        else if (line.end == MAX_HEIGHT_INDEX) {
            //! Use a pointer to the start of the column in the frame buffer
            uint8_t *col_ptr = &frame_buffer[0][line.xy];

            //! Iterate through the y range of the current line (end is always SSD1306_WIDTH - 1)
            for (uint8_t y = 0; y <= MAX_HEIGHT_INDEX; y++) {
                //! Fetch precomputed page and bitmask for y
                uint8_t page = page_masks[y].page;
                uint8_t bitmask = page_masks[y].bitmask;
                col_ptr[page * SSD1306_WIDTH] |= bitmask;   // set the bits in frame_buffer
            }
        }
        else {
            //! Iterate through the y range of the current line
            for (uint8_t y = line.start; y <= line.end; y++) {
                //! Fetch precomputed page and bitmask for y
                uint8_t page = page_masks[y].page;
                uint8_t bitmask = page_masks[y].bitmask;
                frame_buffer[page][line.xy] |= bitmask;     // set the bits in frame_buffer
            }
        }
    }

    ssd1306_update_frame();
}

// Draw horizontal lines
void ssd1306_horizontal_lines(const M_Line *lines, uint8_t num_lines) {
    //! Iterate through all requested horizontal lines
    for (uint8_t i = 0; i < num_lines; i++) {
        M_Line line = lines[i];

        //! Fetch precomputed page and bitmask for y
        uint8_t page = page_masks[line.xy].page;
        uint8_t bitmask = page_masks[line.xy].bitmask;

        //! Use a pointer to the start of the row in the frame buffer
        uint8_t *row_ptr = &frame_buffer[page][line.start];

        //! Iterate through the x range of the current line
        // for (uint8_t x = line.start; x <= line.end; x++) {
        //     // Set only the specific bit in the frame buffer
        //     *row_ptr++ |= bitmask;
        // }

        //! Optimization: Process 4 bytes at a time for efficiency
        uint8_t x = line.start;
        uint8_t end_x = line.end;

        while (x + 4 <= end_x) {
            row_ptr[0] |= bitmask;
            row_ptr[1] |= bitmask;
            row_ptr[2] |= bitmask;
            row_ptr[3] |= bitmask;
            row_ptr += 4;
            x += 4;
        }

        //! Process remaining bytes
        while (x <= end_x) {
            *row_ptr++ |= bitmask;
            x++;
        }
    }

    ssd1306_update_frame();
}


// The width of the line is a horizontal property, while the bar_masks array is designed to handle
// the vertical span of the line. Because of this, the width cannot be directly encoded in
// the bar_masks array in a way that would eliminate the need for the drawing function to handle the width.
uint8_t bar_masks2[SSD1306_HEIGHT];

void precompute_bar_masks2() {
    //! Clear the bar_masks array
    memset(bar_masks2, 0, sizeof(bar_masks2));

    for (uint8_t y = 0; y < SSD1306_HEIGHT; y++) {
        uint8_t page = y / 8; // Page index
        uint8_t bitmask = 1 << (y % 8); // Bitmask for the current y position
        bar_masks2[y] |= bitmask; // Store the bitmask
    }
}

void ssd1306_vertical_lines2(const M_Line *lines, uint8_t num_lines, uint8_t width) {
    for (uint8_t i = 0; i < num_lines; i++) {
        M_Line line = lines[i];

        //! Iterate through the width of the line
        for (uint8_t w = 0; w < width; w++) {
            uint8_t x = line.xy + w;

            //! Iterate through the y range of the current line
            for (uint8_t y = line.start; y <= line.end; y++) {
                uint8_t page = y / 8; // Page index
                uint8_t bitmask = bar_masks2[y];
                frame_buffer[page][x] |= bitmask; // Set the bits in the frame buffer
            }
        }
    }

    ssd1306_update_frame();
}



void ssd1306_spectrum(uint8_t band_cnt) {
    precompute_page_masks(true);
    precompute_bar_masks2(3);
    
    M_Line lines[] = {
        {10, 0, 30},
        {20, 0, 55},
        {50, 0, 40},
        {100, 0, 60}
    };
    uint8_t num_lines = sizeof(lines) / sizeof(lines[0]);
    // ssd1306_vertical_lines(lines, num_lines);
    ssd1306_vertical_lines2(lines, num_lines, 3);

    // Define horizontal lines
    M_Line horizontal_lines[] = {
        {5, 20, 50},   // y = 5, from x = 20 to x = 50
        {15, 60, 100}, // y = 10, from x = 60 to x = 100
        {30, 0, 120},   // y = 15, from x = 0 to x = 127
        {50, 30, 100}   // y = 15, from x = 0 to x = 127
    };
    uint8_t num_horizontal_lines = sizeof(horizontal_lines) / sizeof(horizontal_lines[0]);
    ssd1306_horizontal_lines(horizontal_lines, num_horizontal_lines);
}
