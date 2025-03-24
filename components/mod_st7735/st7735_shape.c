#include "st7735_shape.h"

#define MAX_LINE_WIDTH 128  // Maximum expected line width for buffer sizing

void st7735_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                     uint16_t color, M_Spi_Conf *config) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    // Calculate required buffer size for this line
    int buffer_size = dx + 1;  // Maximum possible pixels in a line segment
    uint16_t color_buffer[buffer_size];
    uint16_t buffer_count = 0;
    
    // Track current and previous positions
    int16_t current_x = x0;
    int16_t current_y = y0;
    int16_t prev_y = y0;
    
    // Set initial window (will update as we draw)
    int16_t window_x0 = x0;
    int16_t window_x1 = x0;
    int16_t window_y0 = y0;
    int16_t window_y1 = y0;

    while (1) {
        // Add pixel to buffer
        color_buffer[buffer_count++] = color;
        
        // Update window boundaries
        if (current_x < window_x0) window_x0 = current_x;
        if (current_x > window_x1) window_x1 = current_x;
        if (current_y < window_y0) window_y0 = current_y;
        if (current_y > window_y1) window_y1 = current_y;

        // Check for Y change or buffer full
        if (current_y != prev_y || buffer_count >= buffer_size) {
            // Flush the buffer
            st7735_set_address_window(window_x0, window_y0, window_x1, window_y1, config);
            mod_spi_data((uint8_t*)color_buffer, buffer_count * 2, config);
            
            // Reset for next segment
            buffer_count = 0;
            window_x0 = window_x1 = current_x;
            window_y0 = window_y1 = current_y;
            prev_y = current_y;
        }

        if (current_x == x1 && current_y == y1) break;
        
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            current_x += sx;
        }
        if (e2 < dy) {
            err += dx;
            current_y += sy;
        }
    }

    // Send any remaining pixels
    if (buffer_count > 0) {
        st7735_set_address_window(window_x0, window_y0, window_x1, window_y1, config);
        mod_spi_data((uint8_t*)color_buffer, buffer_count * 2, config);
    }
}