#include "ssd1306_bitmap.h"

#include "mod_ssd1306.h"
#include "mod_bitmap.h"

unsigned char thermometer[] = {8,20,212,20,20,220,28,28,220,28,62,127,127,127,62,28};  // 8w x 16h
unsigned char droplet[] = {0,4,8,8,24,24,60,60,126,126,255,255,255,255,126,60};  // 8w x 16h
unsigned char sun[] = {1,0,65,4,32,8,3,128,15,224,15,224,31,240,223,246,31,240,15,224,15,224,3,128,32,8,65,4,1,0,0,0};  // 16w x 16h
unsigned char cloud[] = {3,192,7,224,15,240,127,254,255,255,255,255,255,255,127,254};  // 16w x 8h
unsigned char rain[] = {0,0,33,8,66,16,132,32};  // 16w x 4h
unsigned char lightning[] = {0,0,3,224,7,192,15,128,3,192,7,0,6,0,8,0};  // 16w x 8h
unsigned char snow[] = {0,0,0,0,0,16,16,40,40,16,17,0,2,128,1,0};  // 16w x 8h
unsigned char stars2[] = {16,0,16,0,16,0,16,0,40,32,198,32,40,32,16,32,16,80,17,140,16,80,0,32,0,32,0,32,0,32,0,0};  // 16w x 16h
unsigned char blackHoleSun[] = {199,192,159,240,56,56,112,28,96,12,224,14,224,14,224,14,224,14,240,30,120,60,127,252,63,248,31,242,7,198,0,0};  // 16w x 16h
unsigned char degreeSymbol[5] = {112,136,136,136,112};  // 8w x 5h
unsigned char minusSymbol[2] = {3,224};  // 16w x 1h
unsigned char percentSymbol[8] = {64,162,164,72,18,37,69,2};  // 8w x 8h



void rotate_90(const uint8_t *original, uint8_t *rotated, uint16_t width, uint16_t height) {
    // Calculate the number of bytes per row in the original and transposed bitmaps
    uint16_t original_row_bytes = (width + 7) >> 3;      // Bytes per row in the original bitmap
    uint16_t rotated_row_bytes = (height + 7) >> 3;   // Bytes per row in the transposed bitmap
    memset(rotated, 0, rotated_row_bytes * width);

    for (uint16_t y = 0; y < height; y++) {              // Iterate over rows in the original bitmap
        uint16_t orig_row_offset = (y >> 3) * original_row_bytes; // Byte offset for the current row
        uint8_t orig_bit = 1 << (y & 0x07);              // Bit position in the current byte

        for (uint16_t x = 0; x < width; x++) {           // Iterate over columns in the original bitmap
            // Calculate byte and bit positions in the original bitmap
            uint16_t orig_byte = orig_row_offset + x;
            uint16_t trans_byte = (x >> 3) * rotated_row_bytes + (height - 1 - y);
            uint8_t trans_bit = 1 << (x & 0x07);

            // Check if the bit is set in the original and set it in the transposed
            if (original[orig_byte] & orig_bit) {
                rotated[trans_byte] |= trans_bit;
            }
        }
    }
}

void rotate_180(const uint8_t *original, uint8_t *rotated, uint16_t width, uint16_t height) {
    // Calculate the number of bytes per row in the original and rotated bitmaps
    uint16_t row_bytes = (width + 7) >> 3; // Bytes per row

    // Initialize rotated bitmap to all zeros
    memset(rotated, 0, row_bytes * height);

    // Perform the rotation
    for (uint16_t y = 0; y < height; y++) {              // Iterate over rows in the original bitmap
        const uint8_t *orig_row = original + (y * row_bytes); // Pointer to the current row
        uint8_t *rot_row = rotated + ((height - 1 - y) * row_bytes); // Pointer to the rotated row

        for (uint16_t x = 0; x < width; x++) {           // Iterate over columns in the original bitmap
            // Calculate byte and bit positions in the original bitmap
            uint16_t orig_byte = x >> 3;
            uint8_t orig_bit = 1 << (x & 0x07);

            // Calculate byte and bit positions in the rotated bitmap
            uint16_t rot_byte = (width - 1 - x) >> 3;
            uint8_t rot_bit = 1 << ((width - 1 - x) & 0x07);

            // Check if the bit is set in the original and set it in the rotated
            if (orig_row[orig_byte] & orig_bit) {
                rot_row[rot_byte] |= rot_bit;
            }
        }
    }
}

static uint8_t reverse_byte(uint8_t byte) {
    uint8_t reversed_byte = 0;
    for (uint8_t i = 0; i < 8; i++) {
        reversed_byte |= ((byte >> i) & 0x01) << (7 - i);
    }
    return reversed_byte;
}

void ssd1306_draw_bitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height, int8_t orientation) {
    if (orientation == 0) {
        // Column-wise orientation (default SSD1306 format)
        for (uint8_t col = 0; col < width; col++) {
            for (uint8_t page = 0; page < (height + 7) / 8; page++) {
                uint8_t byte = bitmap[col * ((height + 7) / 8) + page]; // Column-wise data
                byte = reverse_byte(byte); // Reverse the byte if needed

                for (uint8_t bit = 0; bit < 8; bit++) {
                    if (byte & (1 << bit)) {
                        uint8_t target_y = y + page * 8 + bit;
                        if (target_y >= SSD1306_HEIGHT) continue; // Skip if out of bounds
                        uint8_t target_page = page_masks[target_y].page;
                        uint8_t target_bitmask = page_masks[target_y].bitmask;
                        frame_buffer[target_page][x + col] |= target_bitmask;
                    }
                }
            }
        }
    } else if (orientation == 1) {
        // Row-wise orientation (8 horizontal pixels per byte)
        for (uint8_t row = 0; row < height; row++) {
            for (uint8_t col = 0; col < (width + 7) / 8; col++) {
                uint8_t byte = bitmap[row * ((width + 7) / 8) + col]; // Row-wise data
                byte = reverse_byte(byte); // Reverse the byte if needed

                for (uint8_t bit = 0; bit < 8; bit++) {
                    if (byte & (1 << bit)) {
                        uint8_t target_x = x + col * 8 + bit;
                        uint8_t target_y = y + row;
                        if (target_x >= SSD1306_WIDTH || target_y >= SSD1306_HEIGHT) continue; // Skip if out of bounds
                        uint8_t target_page = page_masks[target_y].page;
                        uint8_t target_bitmask = page_masks[target_y].bitmask;
                        frame_buffer[target_page][target_x] |= target_bitmask;
                    }
                }
            }
        }
    }
}

void ssd1306_test_bitmaps() {
    //! Clear the buffer
    memset(frame_buffer, 0, sizeof(frame_buffer));

    //! precompute page masks
    precompute_page_masks();

    // Draw the bitmap at position (10, 10) with column-wise orientation
    ssd1306_draw_bitmap(0, 0, thermometer, 8, 16, 1);
    ssd1306_draw_bitmap(15, 0, droplet, 8, 16, 1);
    ssd1306_draw_bitmap(30, 0, rain, 16, 4, 1);
    ssd1306_draw_bitmap(45, 0, lightning, 16, 8, 1);
    ssd1306_draw_bitmap(60, 0, snow, 16, 8, 1);
    ssd1306_draw_bitmap(75, 0, sun, 16, 16, 1);
    ssd1306_draw_bitmap(90, 0, stars2, 16, 16, 1);


    for (int i=0; i < 9; i++) {
        ssd1306_draw_bitmap(15 * i, 20, ICONS_8x8[i + 81], 8, 8, 1);
        ssd1306_draw_bitmap(15 * i, 30, ICONS_8x8[i + 90], 8, 8, 1);
        ssd1306_draw_bitmap(15 * i, 40, ICONS_8x8[i + 99], 8, 8, 1);
    }

    uint8_t icon[8];
    memcpy(icon, ICONS_8x8[0], sizeof(icon));

    rotate_90(ICONS_8x8[0], icon, 8, 8);
    ssd1306_draw_bitmap(0, 55, icon, 8, 8 , 1);

    rotate_180(ICONS_8x8[0], icon, 8, 8);
    ssd1306_draw_bitmap(15, 55, icon, 8, 8 , 1);


    //! update frame
    ssd1306_update_frame();
}