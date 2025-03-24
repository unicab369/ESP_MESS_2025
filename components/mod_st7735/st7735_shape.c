#include "st7735_shape.h"
#include "mod_utility.h"

#define MIN(a, b) ((a < b) ? a : b)

#define OUTPUT_BUFFER_SIZE 100  // Number of pixels to buffer before sending
uint16_t color_buffer[OUTPUT_BUFFER_SIZE];

#define MAX_SAFE_MEMORY 1024  // 1KB

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
    if (x0 > x1) { uint16_t tmp = x0; x0 = x1; x1 = tmp; }
    uint16_t length = x1 - x0 + 1;

    //! Calculate SAFE buffer size (max 1KB to prevent stack overflow)
    uint16_t safe_pixels = MAX_SAFE_MEMORY / (thickness * 2);  // 2 bytes/pixel
    safe_pixels = (safe_pixels > 0) ? safe_pixels : 1;  // Ensure at least 1 pixel
    
    //! Fill buffer
    uint16_t chunk_buffer[safe_pixels * thickness];     // allocate buffer
    uint32_t *buf32 = (uint32_t*)chunk_buffer;
    uint32_t color32 = (color << 16) | color;
    uint16_t total_words = (safe_pixels * thickness) / 2;

    for (uint16_t word = 0; word < total_words; word++) {
        buf32[word] |= color32; // Writes 2 pixels per iteration
    }
    
    //! Handle odd pixels
    if ((safe_pixels * thickness) % 2) {
        chunk_buffer[safe_pixels * thickness - 1] |= color;
    }

    //! Draw in safe chunks
    for (uint16_t current_x = x0; current_x <= x1; current_x += safe_pixels) {
        uint16_t chunk_width = MIN(safe_pixels, x1 - current_x + 1);
        
        //# Set window
        st7735_set_address_window(
            current_x, y,
            current_x + chunk_width - 1, y + thickness - 1, config
        );
        
        //# Send the buffer
        mod_spi_data((uint8_t*)chunk_buffer, chunk_width * thickness * 2, config);
    }
}

//# Draw vertical line
void st7735_draw_verLine(
    uint16_t x, uint16_t y0, uint16_t y1,
    uint16_t color, uint8_t thickness,
    M_Spi_Conf *config
) {
    if (y0 > y1) { uint16_t tmp = y0; y0 = y1; y1 = tmp; }
    uint16_t length = y1 - y0 + 1;

    //! Calculate safe buffer size (same 1KB limit)
    uint16_t safe_pixels = MAX_SAFE_MEMORY / (thickness * 2);
    safe_pixels = (safe_pixels > 0) ? safe_pixels : 1;
    uint16_t chunk_buffer[safe_pixels * thickness];

    //! Fill buffer
    uint32_t *buf32 = (uint32_t*)chunk_buffer;
    uint32_t color32 = (color << 16) | color;
    uint16_t total_words = (safe_pixels * thickness) / 2;
    for (uint16_t word = 0; word < total_words; word++) {
        buf32[word] |= color32; // Write 2 pixels per iteration
    }

    //! Handle odd pixels
    if ((safe_pixels * thickness) % 2) {
        chunk_buffer[safe_pixels * thickness - 1] |= color;
    }

    //! Draw vertical strips in chunks
    for (uint16_t current_y = y0; current_y <= y1; current_y += safe_pixels) {
        uint16_t chunk_height = MIN(safe_pixels, y1 - current_y + 1);
        
        //# Set window
        st7735_set_address_window(
            x, current_y,
            x + thickness - 1, current_y + chunk_height - 1, config
        );
        
        //# Send the buffer
        mod_spi_data((uint8_t*)chunk_buffer, chunk_height * thickness * 2, config);
    }
}