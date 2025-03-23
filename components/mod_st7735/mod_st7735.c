#include "mod_st7735.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mod_bitmap.h"

#define ST7735_SIZE_1p8IN 1

#ifdef ST7735_SIZE_1p8IN
    #define ST7735_WIDTH 128
    #define ST7735_HEIGHT 160
#else
    #define ST7735_WIDTH 80
    #define ST7735_HEIGHT 160
#endif

#define DISPLAY_NUM_PIXELS (ST7735_WIDTH * ST7735_HEIGHT)

static uint16_t tft_buffer[DISPLAY_NUM_PIXELS];
static uint16_t BACKGROUND_COLOR = 0x0000;

void st7735_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, M_Spi_Conf *conf) {           
    uint8_t data[] = {0x00, x0, 0x00, x1};
    //! ST7735_CASET
    mod_spi_cmd(0x2A, conf);   
    mod_spi_data(data, 4, conf);

    data[1] = y0;
    data[3] = y1;
    //! ST7735_RASET
    mod_spi_cmd(0x2B, conf);
    mod_spi_data(data, 4, conf);

    //! ST7735_RAMWR
    mod_spi_cmd(0x2C, conf);
}

void st7735_fill_screen(uint16_t color, M_Spi_Conf *conf) {
    //! Set the address window to cover the entire screen
    st7735_set_address_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1, conf);

    //! Precompute the high and low bytes of the color
    uint8_t color_high = color >> 8;
    uint8_t color_low = color & 0xFF;

    const size_t CHUNK_SIZE = 2000;
    uint8_t chunk[CHUNK_SIZE]; // Buffer to hold the chunk

    //! Calculate the total number of pixels
    size_t total_pixels = ST7735_WIDTH * ST7735_HEIGHT;

    //! Fill the screen in chunks
    size_t pixels_sent = 0;
    while (pixels_sent < total_pixels) {
        //! Determine the number of pixels to send in this chunk
        size_t pixels_in_chunk = (total_pixels - pixels_sent > CHUNK_SIZE / 2)
                                ? CHUNK_SIZE / 2
                                : (total_pixels - pixels_sent);

        //! Fill the chunk with the color data
        for (size_t i = 0; i < pixels_in_chunk; i++) {
            chunk[2 * i] = color_high;   // High byte
            chunk[2 * i + 1] = color_low; // Low byte
        }

        //! Send the chunk to the display
        mod_spi_data(chunk, pixels_in_chunk * 2, conf);

        //! Update the number of pixels sent
        pixels_sent += pixels_in_chunk;
    }
}

esp_err_t st7735_init(uint8_t rst, M_Spi_Conf *conf) {
    //! Set the RST pin
    esp_err_t ret = gpio_set_direction(rst, GPIO_MODE_OUTPUT);

    //! Reset the display
    gpio_set_level(rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    //! Send initialization commands
    mod_spi_cmd(0x01, conf); // Software reset
    vTaskDelay(pdMS_TO_TICKS(120));
    mod_spi_cmd(0x11, conf); // Sleep out
    vTaskDelay(pdMS_TO_TICKS(120));
    
    //! Set color mode
    mod_spi_cmd(0x3A, conf);
    mod_spi_data((uint8_t[]){0x05}, 1, conf); // 16-bit color

    //! Start display
    mod_spi_cmd(0x29, conf); // Display on

    //! Fill display color
    st7735_fill_screen(BACKGROUND_COLOR, conf);

    return ret;
}


void draw_char(M_TFT_Text *model, uint8_t x, uint8_t y, char c, M_Spi_Conf *config) {
    //! Calculate the size of the frame buffer
    uint16_t frame_buffer_size = model->font_width * model->font_height * 2; // 2 bytes per pixel (RGB565)
    uint8_t tft_frame_buffer[frame_buffer_size]; // Frame buffer for the character
    int buff_idx = 0;

    //! Get the font data for the current character
    const uint8_t *char_data = &model->font[(c - 32) * model->font_width * ((model->font_height + 7) / 8)];

    //! Render the character into the frame buffer
    for (int j = 0; j < model->font_height; j++) { // Rows
        for (int i = 0; i < model->font_width; i++) { // Columns
            if (char_data[(i + (j / 8) * model->font_width)] & (1 << (j % 8))) {
                //! Pixel is part of the character (foreground color)
                tft_frame_buffer[buff_idx++] = model->color >> 8;   // High byte of RGB565
                tft_frame_buffer[buff_idx++] = model->color & 0xFF; // Low byte of RGB565
            } else {
                //! Pixel is not part of the character (background color)
                tft_frame_buffer[buff_idx++] = BACKGROUND_COLOR >> 8;   // High byte of RGB565
                tft_frame_buffer[buff_idx++] = BACKGROUND_COLOR & 0xFF; // Low byte of RGB565
            }
        }
    }

    //! Set the address window to cover the character area
    st7735_set_address_window(x, y, x + model->font_width - 1, y + model->font_height - 1, config);

    //! Send the frame buffer to the display in one transaction
    mod_spi_data(tft_frame_buffer, frame_buffer_size, config);
}


void st7735_draw_text(M_TFT_Text *model, M_Spi_Conf *config) {
    if (!model || !config || !model->text || !model->font) return; // Error handling

    uint8_t current_x = model->x;               // Current x position
    uint8_t current_y = model->y;               // Current y position
    uint8_t start_x = model->x;                 // Initial x position for each line
    const char *text = model->text;             // Pointer to the text
    uint8_t font_width = model->font_width;     // Font width
    uint8_t font_height = model->font_height;   // Font height
    uint8_t char_spacing = model->char_spacing; // Spacing between characters

    while (*text) {
        //! Handle newline character
        if (*text == '\n') {
            current_x = start_x; // Reset x to the start position
            current_y += font_height + 1; // Move y to the next line (font height + 1 pixel spacing)

            //! Stop rendering if the display height is exceeded
            if (current_y + font_height > ST7735_HEIGHT) break;

            text++; // Move to the next character
            continue;
        }

        // Calculate the width of the current character
        uint8_t char_width = font_width + char_spacing;

        //! Handle wrapping
        if (current_x + char_width > ST7735_WIDTH) {
            if (model->page_wrap) {
                if (model->word_wrap && *text != ' ') {
                    //! Move the entire word to the next line
                    current_x = start_x; // Reset x to the start position
                    current_y += font_height + 1; // Move y to the next line

                    //! Stop rendering if the display height is exceeded
                    if (current_y + font_height > ST7735_HEIGHT) break;
                } else {
                    //! Break the word and wrap to the next line
                    current_x = start_x; // Reset x to the start position
                    current_y += font_height + 1; // Move y to the next line

                    //! Stop rendering if the display height is exceeded
                    if (current_y + font_height > ST7735_HEIGHT) break;
                }
            } else {
                //! Handle truncation: stop rendering
                break;
            }
        }

        //! Render the current character
        draw_char(model, current_x, current_y, *text, config);

        //! Move to the next character position
        current_x += char_width;    // Move x by character width + spacing
        text++;                     // Move to the next character
    }
}