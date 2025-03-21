
#include <unistd.h>
#include <stdint.h>


typedef struct {
    uint8_t pos;         // X or y position of the line
    uint8_t start;      // Start position of the line
    uint8_t end;        // End position of the line
} M_Line;

typedef struct {
    uint8_t page;
    uint8_t bitmask;
} M_Page_Mask;


void ssd1306_spectrum(uint8_t num_bands);