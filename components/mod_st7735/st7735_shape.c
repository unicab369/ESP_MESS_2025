#include "st7735_shape.h"


#define LINE_BUFFER_SIZE 32


typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t color;
} PixelData;

PixelData pixel_buffer[LINE_BUFFER_SIZE];

//# Bresenham's line algorithm 
void st7735_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
         uint16_t color, M_Spi_Conf *config) {
    // Bresenham's line algorithm variables
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int e2;
    
    // Calculate the bounding box of the line
    int16_t min_x = (x0 < x1) ? x0 : x1;
    int16_t max_x = (x0 < x1) ? x1 : x0;
    int16_t min_y = (y0 < y1) ? y0 : y1;
    int16_t max_y = (y0 < y1) ? y1 : y0;
    
    // Pixel buffer and tracking
    uint16_t buffer_count = 0;

    // // Set address window for the entire line once
    // st7735_set_address_window(min_x, min_y, max_x, max_y, config);

    while (1) {
        // Store pixel position and color
        pixel_buffer[buffer_count].x = x0;
        pixel_buffer[buffer_count].y = y0;
        pixel_buffer[buffer_count].color = color;
        buffer_count++;

        //# Send buffer when full
        if (buffer_count >= LINE_BUFFER_SIZE) {
            for (int i = 0; i < buffer_count; i++) {
                uint8_t color_data[2] = {pixel_buffer[i].color >> 8, pixel_buffer[i].color & 0xFF};
                
                st7735_set_address_window(pixel_buffer[i].x, pixel_buffer[i].y, 
                                        pixel_buffer[i].x, pixel_buffer[i].y, config);
                mod_spi_data(color_data, 2, config);
            }
            buffer_count = 0;
        }


        // Check if line is complete
        if (x0 == x1 && y0 == y1) break;
        
        // Update error term and coordinates
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }

    //# Send remaining pixels
    for (int i = 0; i < buffer_count; i++) {
        uint8_t color_data[2] = {pixel_buffer[i].color >> 8, pixel_buffer[i].color & 0xFF};

        st7735_set_address_window(pixel_buffer[i].x, pixel_buffer[i].y, 
                                    pixel_buffer[i].x, pixel_buffer[i].y, config);
        mod_spi_data(color_data, 2, config);
    }
}