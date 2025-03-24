#include "st7735_shape.h"
#include "mod_utility.h"

#define LINE_BUFFER_SIZE 500  // Number of pixels to buffer before sending

void st7735_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                     uint16_t color, M_Spi_Conf *config) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    uint16_t buffer_count = 0;
    uint16_t color_buffer[LINE_BUFFER_SIZE];
    memset(color_buffer, 0, sizeof(color_buffer));

    // Track current position and window bounds
    int16_t current_x = x0;
    int16_t current_y = y0;
    int16_t min_x = x0, max_x = x0;
    int16_t min_y = y0, max_y = y0;

    while (1) {
        if (current_x == x1 && current_y == y1) break;
        color_buffer[buffer_count++] = color;

        // When buffer is full, send all pixels
        if (buffer_count >= LINE_BUFFER_SIZE) {
            // Update window bounds
            if (current_x < min_x) min_x = current_x;
            if (current_x > max_x) max_x = current_x;
            if (current_y < min_y) min_y = current_y;
            if (current_y > max_y) max_y = current_y;

            printf("window [%d, %d, %d, %d]\n", min_x, min_y, max_x, max_y);
            printf("New dx segment (Y change):\n");
            for (int i = 0; i < buffer_count; i++) {
                print_hex_debug(color_buffer[i]);
                if ((i+1) % dx == 0) printf(".\n");
            }
            printf("\n\n");

            st7735_set_address_window(min_x, min_y, max_x, max_y, config);
            mod_spi_data((uint8_t*)color_buffer, buffer_count * 2, config);
            
            // Reset for next segment
            buffer_count = 0;
            min_x = current_x;
            max_x = current_x;
            min_y = current_y;
            max_y = current_y;
        }

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

    // Send remaining pixels
    if (buffer_count > 0) {
        st7735_set_address_window(min_x, min_y, max_x, max_y, config);
        mod_spi_data((uint8_t*)color_buffer, buffer_count * 2, config);
    }
}