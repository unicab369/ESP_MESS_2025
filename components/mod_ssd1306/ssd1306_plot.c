#include "ssd1306_plot.h"

#include "mod_ssd1306.h"
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

int16_t samples[] = {
    10, 20, 30, 40, 50, 60, 35, 20, 63, 10, 11, 12, 13, 63, 15, 16,
    10, 20, 30, 63, 50, 60, 63, 63, 63, 10, 63, 12, 13, 14, 15, 16
};

// FFT-based spectrum analyzer core
void ssd1306_spectrum(uint8_t num_band) {
    if (ssd1306_print_mode != 2) return;

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

    //! Draw bars
    for (int i = 0; i < num_band; i++) {
        ssd1306_vertical_line(&bands[i], 4, true);
        ssd1306_horizontal_line(&bands[i], 4, true);
    }

    //! Draw rectangle
    ssd1306_rectangle(30, 10, 70, 30);

    //! Draw line
    ssd1306_draw_line(20, 20, 80, 50, 3);

    //! Draw triangle
    ssd1306_triangle(55, 25, 90, 20, 70, 60);

    //! Update the display
    ssd1306_update_frame();
}