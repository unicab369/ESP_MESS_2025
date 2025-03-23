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





//! Struct to track rendering state
typedef struct {
    uint16_t accumulated_size; // Accumulated size of characters on the current line (in bytes)
    uint8_t current_x;         // Current x position
    uint8_t current_y;         // Current y position
    uint8_t render_start_x;    // Starting x position of the current buffer
    uint8_t render_end_x;      // Ending x position of the current buffer
} RenderState;

void handle_buffer(RenderState *state, uint16_t line_idx, uint8_t font_height, uint16_t buffer_size) {
    if (state->accumulated_size > 0) {
        printf("Line %d: Size = %d bytes,\t Window: [x=%d, y=%d] to [x=%d, y=%d]\n", 
                line_idx, buffer_size, state->render_start_x, state->current_y, 
                state->render_end_x, state->current_y + font_height - 1);
    }
}

void reset_render_state(RenderState *state, uint8_t start_x, uint8_t font_height) {
    state->accumulated_size = 0;
    state->current_x = start_x;
    state->current_y += font_height + 1; // Move to the next line
    state->render_start_x = start_x;
    state->render_end_x = start_x;
}

void draw_char(M_TFT_Text *model, uint16_t *buff, char c) {
    //! Get the font data for the current character
    const uint8_t *char_data = &model->font[(c - 32) * model->font_width * ((model->font_height + 7) / 8)];
    int buff_idx = 0;
    
    //! Render the character into the frame buffer
    for (int j = 0; j < model->font_height; j++) { // Rows
        for (int i = 0; i < model->font_width; i++) { // Columns
            if (char_data[(i + (j / 8) * model->font_width)] & (1 << (j % 8))) {
                //! Pixel is part of the character (foreground color)
                buff[buff_idx++] = model->color; // RGB565 color
            } else {
                //! Pixel is not part of the character (background color)
                buff[buff_idx++] = BACKGROUND_COLOR; // RGB565 background color
            }
        }
    }
}

//! Main function to draw text
void st7735_draw_text(M_TFT_Text *model, M_Spi_Conf *config) {
    if (!model || !config || !model->text || !model->font) return; // Error handling

    //! Initialize rendering state
    RenderState state = {
        .accumulated_size = 0,
        .current_x = model->x,
        .current_y = model->y,
        .render_start_x = model->x,
        .render_end_x = model->y
    };

    uint8_t start_x = model->x;                 // Initial x position for each line
    uint8_t font_width = model->font_width;     // Font width
    uint8_t font_height = model->font_height;   // Font height
    uint8_t char_spacing = model->char_spacing; // Spacing between characters
    const char *text = model->text;             // Pointer to the text

    uint16_t line_idx = 0; // Current line index

    while (*text) {
        //! Handle newline character
        if (*text == '\n') {
            //! Print the remaining buffer if it has data
            handle_buffer(&state, line_idx, font_height, state.accumulated_size);

            //! Reset for the next line
            reset_render_state(&state, start_x, font_height);
            line_idx++;

            //! Stop rendering if the display height is exceeded
            if (state.current_y + font_height > ST7735_HEIGHT) break;

            text++; // Move to the next character
            continue;
        }

        //! Find the end of the current word
        const char *word_end = text;
        while (*word_end && *word_end != ' ' && *word_end != '\n') {
            word_end++;
        }

        //! Calculate the width of the current word
        uint8_t word_width = 0;
        const char *temp = text;
        while (temp < word_end) {
            word_width += font_width + char_spacing; // Add character width + spacing
            temp++;
        }

        //! Handle word wrapping
        if (state.current_x + word_width > ST7735_WIDTH) {
            if (model->word_wrap) {
                //! Print the remaining buffer if it has data
                handle_buffer(&state, line_idx, font_height, state.accumulated_size);

                //! Reset for the next line
                reset_render_state(&state, start_x, font_height);
                line_idx++;

                //! Stop rendering if the display height is exceeded
                if (state.current_y + font_height > ST7735_HEIGHT) break;
            } else {
                //! Break the word and print the part that fits
                while (text < word_end) {
                    uint8_t char_width = font_width + char_spacing;

                    //! Handle wrapping for individual characters
                    if (state.current_x + char_width > ST7735_WIDTH) {
                        //! Print the remaining buffer if it has data
                        handle_buffer(&state, line_idx, font_height, state.accumulated_size);

                        //! Reset for the next line
                        reset_render_state(&state, start_x, font_height);
                        line_idx++;

                        //! Stop rendering if the display height is exceeded
                        if (state.current_y + font_height > ST7735_HEIGHT) break;
                    }

                    //! Accumulate the size of the current character
                    state.accumulated_size += model->font_width * model->font_height * 2; // Each pixel is 2 bytes (RGB565)

                    //! Update the rendering window
                    state.render_end_x = state.current_x + font_width;

                    //! Check if the accumulated size exceeds 500 bytes
                    if (state.accumulated_size >= 500) {
                        handle_buffer(&state, line_idx, font_height, 500);
                        state.accumulated_size -= 500; // Keep the remaining bytes for the next print

                        //! Update the rendering window for the next chunk
                        state.render_start_x = state.render_end_x;
                        state.render_end_x = state.render_start_x;
                    }

                    //! Render the current character
                    uint16_t buff_len = model->font_width * model->font_height; // Number of pixels
                    uint16_t frame_buff[buff_len]; // Frame buffer for the character (uint16_t for RGB565)
                    draw_char(model, frame_buff, *text);

                    //! Set the address window to cover the character area
                    st7735_set_address_window(state.current_x, state.current_y, 
                        state.current_x + model->font_width - 1, state.current_y + model->font_height - 1, config);

                    //! Send the frame buffer to the display in one transaction
                    mod_spi_data((uint8_t *)frame_buff, buff_len * 2, config); // Multiply by 2 for uint16_t size

                    //! Move to the next character position
                    state.current_x += char_width;    // Move x by character width + spacing
                    text++;                           // Move to the next character
                }

                continue; // Skip the space handling below
            }
        }

        //! Render the current word
        while (text < word_end) {
            //! Accumulate the size of the current character
            state.accumulated_size += model->font_width * model->font_height * 2; // Each pixel is 2 bytes (RGB565)

            //! Update the rendering window
            state.render_end_x = state.current_x + font_width;

            //! Check if the accumulated size exceeds 500 bytes
            if (state.accumulated_size >= 500) {
                handle_buffer(&state, line_idx, font_height, 500);
                state.accumulated_size -= 500; // Keep the remaining bytes for the next print

                //! Update the rendering window for the next chunk
                state.render_start_x = state.render_end_x;
                state.render_end_x = state.render_start_x;
            }

            //! Render the current character
            uint16_t buff_len = model->font_width * model->font_height; // Number of pixels
            uint16_t frame_buff[buff_len]; // Frame buffer for the character (uint16_t for RGB565)
            draw_char(model, frame_buff, *text);

            //! Set the address window to cover the character area
            st7735_set_address_window(state.current_x, state.current_y,
                state.current_x + model->font_width - 1, state.current_y + model->font_height - 1, config);

            //! Send the frame buffer to the display in one transaction
            mod_spi_data((uint8_t *)frame_buff, buff_len * 2, config); // Multiply by 2 for uint16_t size

            //! Move to the next character position
            state.current_x += font_width + char_spacing;     // Move x by character width + spacing
            text++;                                           // Move to the next character
        }

        //! Skip the space character after the word
        if (*text == ' ') {
            state.current_x += font_width + char_spacing; // Add space between words
            text++;
        }
    }

    //! Print the remaining buffer if it has data
    handle_buffer(&state, line_idx, font_height, state.accumulated_size);
}