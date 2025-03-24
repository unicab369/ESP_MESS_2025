#include "st7735_shape.h"
#include "mod_utility.h"

#define OUTPUT_BUFFER_SIZE 100  // Number of pixels to buffer before sending

uint16_t color_buffer[OUTPUT_BUFFER_SIZE];


//# Draw line
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
    uint16_t color_buffer[OUTPUT_BUFFER_SIZE];
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
        if (buffer_count >= OUTPUT_BUFFER_SIZE) {
            // Update window bounds
            if (current_x < min_x) min_x = current_x;
            if (current_x > max_x) max_x = current_x;
            if (current_y < min_y) min_y = current_y;
            if (current_y > max_y) max_y = current_y;

            printf("window [%d, %d, %d, %d]\n", min_x, min_y, max_x, max_y);
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
        printf("***window [%d, %d, %d, %d]\n", min_x, min_y, max_x, max_y);
        printf("New dx segment (Y change):\n");
        for (int i = 0; i < buffer_count; i++) {
            print_hex_debug(color_buffer[i]);
            if ((i+1) % dx == 0) printf(".\n");
        }
        printf("\n\n");

        st7735_set_address_window(min_x, min_y, max_x, max_y, config);
        mod_spi_data((uint8_t*)color_buffer, buffer_count * 2, config);
    }
}


//# Draw horizontal line
void st7735_draw_horLine(
    uint16_t y, uint16_t x0, uint16_t x1, 
    uint16_t color, uint8_t thickness, 
    M_Spi_Conf *config
) {
    // Buffer for vertical strips (aligned for 32-bit writes)
    uint16_t chunk_buffer[OUTPUT_BUFFER_SIZE] __attribute__((aligned(4)));
    uint16_t pixels_per_chunk = OUTPUT_BUFFER_SIZE / thickness;
    uint32_t color32 = (color << 16) | color;       // Pack 2 pixels into 32 bits
    uint32_t *buf32 = (uint32_t*)chunk_buffer;

    // Fill buffer with thickness-striped pixels
    for (uint16_t i = 0; i < pixels_per_chunk / 2; i++) {
        for (uint8_t t = 0; t < thickness; t++) {
            buf32[i * thickness + t] = color32; // 2 pixels per write
        }
    }

    // Handle odd pixels_per_chunk
    if (pixels_per_chunk % 2) {
        for (uint8_t t = 0; t < thickness; t++) {
            chunk_buffer[(pixels_per_chunk - 1) * thickness + t] = color;
        }
    }

    // Draw in chunks
    uint16_t start_x = x0 < x1 ? x0 : x1;
    uint16_t end_x = x0 < x1 ? x1 : x0;
    uint16_t remaining = end_x - start_x + 1;
    uint16_t current_x = start_x;
    
    while (remaining > 0) {
        uint16_t chunk_width = (remaining > pixels_per_chunk) ? pixels_per_chunk : remaining;
        
        //# set address window
        st7735_set_address_window(current_x, y,
            current_x + chunk_width - 1, y + thickness - 1,
            config
        );
        
        //# send spi data
        mod_spi_data((uint8_t*)chunk_buffer, 
            chunk_width * thickness * 2, // 2 bytes per pixel
            config
        );
        
        current_x += chunk_width;
        remaining -= chunk_width;
    }
}