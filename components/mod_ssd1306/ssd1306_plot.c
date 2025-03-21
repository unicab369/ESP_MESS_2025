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

// The width of the line is a horizontal property, while the bar_masks array is designed to handle
// the vertical span of the line. Because of this, the width cannot be directly encoded in
// the bar_masks array in a way that would eliminate the need for the drawing function to handle the width.

uint8_t frame_buffer[SSD1306_PAGES][SSD1306_WIDTH] = {0};

static uint8_t bar_masks[SSD1306_HEIGHT][SSD1306_PAGES] = {0};  // Lookup table for bar masks

void precompute_bar_masks(uint8_t height, bool flipped) {
    for (uint8_t h = 0; h < 64; h++) {
        uint8_t pages = h >> 3;
        uint8_t remainder = h & 0x07;

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

void ssd1306_vertical_lines_width2(uint8_t x, uint8_t height, uint8_t width, uint8_t space) {
    uint16_t x_value = x * (width + space);
    if (x_value + width > SSD1306_WIDTH) return;

    // Draw the bar
    for (uint8_t w = 0; w < width; w++) {
        for (uint8_t p = 0; p < SSD1306_PAGES; p++) {
            frame_buffer[p][x_value + w] = bar_masks[height][p];
        }
    }
}

// FFT-based spectrum analyzer core
void ssd1306_spectrum2(uint8_t band_cnt) {
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
        ssd1306_vertical_lines_width2(b, band_heights[b], 5, 1);
    }

    //! Update the display
    ssd1306_update_frame();
}


M_Page_Mask page_masks[SSD1306_HEIGHT];

void precompute_page_masks(uint8_t inverted) {
    for (uint8_t y = 0; y < SSD1306_HEIGHT; y++) {
        uint8_t y_value = inverted ? SSD1306_HEIGHT - 1 - y : y;
        page_masks[y].page       = y_value >> 3;             // (y_value / 8)
        page_masks[y].bitmask    = 1 << (y_value & 0x07);    // (y_value % 8)
    }
}

void ssd1306_horizontal_line(const M_Line *line) {
    //! Fetch precomputed page and bitmask for y
    uint8_t page = page_masks[line->pos].page;
    uint8_t bitmask = page_masks[line->pos].bitmask;

    //! Use a pointer to the start of the row in the frame buffer
    uint8_t x = line->start;
    uint8_t end_x = line->end;
    uint8_t *row_ptr = &frame_buffer[page][x];

    //! Iterate through the x range of the current line
    // for (uint8_t i = line.start; i <= line.end; i++) {
    //     // Set only the specific bit in the frame buffer
    //     *row_ptr++ |= bitmask;
    // }

    //! Optimization: Process 4 bytes at a time for efficiency
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

void ssd1306_vertical_line(const M_Line *line, uint8_t width) {
    uint8_t start_page = page_masks[line->start].page;
    uint8_t end_page = page_masks[line->end].page;

    //! Iterate through the width of the line
    for (uint8_t w = 0; w < width; w++) {
        uint8_t x_value = line->pos + w;

        // //! Iterate through the y range of the current line
        // for (uint8_t y = line.start; y <= line.end; y++) {
        //     //! Fetch precomputed page and bitmask for y
        //     uint8_t page = page_masks[y].page;
        //     frame_buffer[page][x_value] |= page_masks[y].bitmask;     // set the bits in frame_buffer
        //     printf("page: %u, x: %u, bitmask: %u\n", page, x_value, page_masks[y].bitmask);
        // }

        //! Optimization: drawing segments by page
        //! Handle full pages: page inbetween start_page and end_page are full page
        for (uint8_t page = start_page + 1; page < end_page; page++) {
            frame_buffer[page][x_value] |= 0xFF; // Set all bits in the page
        }

        //! Handle the start page
        uint8_t bitmask = 0;
        if (line->start % 8 == 0) {
            // Start page is full
            bitmask |= 0xFF;
        } else {
            // Start page is partial
            for (uint8_t y = line->start; y <= line->end && page_masks[y].page == start_page; y++) {
                bitmask |= page_masks[y].bitmask;
                break;
            }
        }
        frame_buffer[start_page][x_value] |= bitmask;

        //! Handle the end page
        if (end_page != start_page) {
            bitmask = 0;
            if ((line->end + 1) % 8 == 0) {
                // End page is full
                bitmask |= 0xFF;
            } else {
                // End page is partial
                for (uint8_t y = line->start; y <= line->end && page_masks[y].page == end_page; y++) {
                    bitmask |= page_masks[y].bitmask;
                    break;
                }
            }
            frame_buffer[end_page][x_value] |= bitmask;
        }
    }
}

void ssd1306_spectrum(uint8_t band_cnt) {
    precompute_page_masks(false);

    M_Line ver_lines[] = {
        {10, 0, 40},
        {40, 0, 55},
        {65, 0, 40},
        {90, 0, 60}
    };
    uint8_t num_lines = sizeof(ver_lines) / sizeof(ver_lines[0]);

    for (int i = 0; i < num_lines; i++) {
        ssd1306_vertical_line(&ver_lines[i], 3);
    }

    // Define horizontal lines
    M_Line horz_lines[] = {
        {5, 20, 50},   // y = 5, from x = 20 to x = 50
        {15, 60, 100}, // y = 10, from x = 60 to x = 100
        {30, 0, 120},   // y = 15, from x = 0 to x = 127
        {50, 30, 100}   // y = 15, from x = 0 to x = 127
    };
    uint8_t num_horz_lines = sizeof(horz_lines) / sizeof(horz_lines[0]);

    for (int i = 0; i < num_horz_lines; i++) {
        ssd1306_horizontal_line(&horz_lines[i]);
    }


    ssd1306_update_frame();
}
