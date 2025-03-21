#include "ssd1306_plot.h"

#include "mod_ssd1306.h"
#include <stdio.h>
#include <string.h>

#include <mod_utility.h>

uint8_t frame_buffer[SSD1306_PAGES][SSD1306_WIDTH] = {0};
M_Page_Mask page_masks[SSD1306_HEIGHT];


void precompute_page_masks() {
    for (uint8_t y = 0; y < SSD1306_HEIGHT; y++) {
        page_masks[y].page       = y >> 3;             // (y / 8)
        page_masks[y].bitmask    = 1 << (y & 0x07);    // (y % 8)
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

void ssd1306_vertical_line(const M_Line *line, uint8_t width, uint8_t inverted) {
    uint8_t start_y = inverted ? (SSD1306_HEIGHT - 1 - line->end) : line->start;
    uint8_t end_y = inverted ? (SSD1306_HEIGHT - 1 - line->start) : line->end;

    uint8_t start_page = page_masks[start_y].page;
    uint8_t end_page = page_masks[end_y].page;

    //! Iterate through the width of the line
    for (uint8_t w = 0; w < width; w++) {
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

int16_t samples[] = {
    10, 20, 30, 40, 50, 60, 35, 20, 63, 10, 11, 12, 13, 63, 15, 16,
    10, 20, 30, 63, 50, 60, 63, 63, 63, 10, 63, 12, 13, 14, 15, 16
};

// FFT-based spectrum analyzer core
void ssd1306_spectrum(uint8_t num_band) {
    // Simple energy calculation per band
    M_Line bands[num_band];
    memset(bands, 0, sizeof(bands));

    uint16_t sample_len = sizeof(samples) / sizeof(samples[0]);
    uint16_t samp_per_band = sample_len / num_band;

    for (int i = 0; i < num_band; i++) {
        int start_index = i * samp_per_band;
        int end_index = start_index + samp_per_band;

        uint16_t sum = 0;
        for (int i = start_index; i < end_index; i++) {
            sum += abs(samples[i]); // Use absolute value as magnitude
        }

        // Calculate the average magnitude for the bin
        bands[i].pos = i * 5;
        bands[i].end = sum / samp_per_band;
    }

    //! Clear the buffer
    memset(frame_buffer, 0, sizeof(frame_buffer));

     //! precompute page masks
    precompute_page_masks();

    //! Draw all bars
    for (int i = 0; i < num_band; i++) {
        ssd1306_vertical_line(&bands[i], 3, false);
    }

    //! Draw Horizontal lines
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

    //! Update the display
    ssd1306_update_frame();
}
