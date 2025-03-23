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


static void handle_buffer(M_Render_State *state, M_Spi_Conf *config) {
    if (state->char_buff_count <= 0) return;

    //! Calculate render_end_x
    uint8_t width_offset = state->char_buff_count * state->font_width;
    uint8_t spacing_offset = state->char_buff_count * state->font_spacing;
    uint8_t render_end_x = state->render_start_x + width_offset + spacing_offset;
    uint8_t render_end_y = state->current_y + state->font_height - 1;

    //! Print the buffer details
    printf("L%d: %d char,\t [x=%d, y=%d] to [x=%d, y=%d]", 
        state->line_idx, state->char_buff_count,
        // start position
        state->render_start_x, state->current_y, 
        // end position
        render_end_x, render_end_y
    );

    //! Print the accumulated characters
    printf("\t\t");
    for (uint8_t i = 0; i < state->char_buff_count; i++) {
        printf("%c", state->char_buff_data[i]);
    }
    printf("\n");

    // //! Render the
    // uint16_t buff_len = state->char_buff_count * state->font_width * state->font_height;
    // uint16_t frame_buff[buff_len];

    // for (int i=0; i<state->char_buff_count; i++) {
        
    // }

    // st7735_set_address_window(state->render_start_x, state->current_y, render_end_x, render_end_y, config);
    // mod_spi_data((uint8_t *)frame_buff, buff_len * 2, config);

    //! Reset the accumulated character count and update render_start_x
    state->char_buff_count = 0;

    // printf("current_x: %d, spaccing_offset: %d\n", state->current_x, spacing_offset);
    state->render_start_x = render_end_x;
}


static bool reset_render_state(M_Render_State *state) {
    //! Reset state for the next line
    state->char_buff_count = 0;                 // Clear the accumulated characters
    state->current_x = 0;                       // Reset x to the start position
    state->render_start_x = 0;                  // Reset render_start_x to the start position
    state->current_y += state->font_height + 1; // Move y to the next line

    //! Move to the next line
    state->line_idx++;

    //! Check if current_y is out of bounds
    return state->current_y + state->font_height > ST7735_HEIGHT;
}


void st7735_draw_text(M_TFT_Text *model, M_Spi_Conf *config) {
    if (!model || !config || !model->text || !model->font) return; // Error handling

    uint8_t font_width = model->font_width;     // Font width
    uint8_t char_spacing = model->char_spacing; // Spacing between characters
    const char *text = model->text;             // Pointer to the text

    //! Initialize rendering state
    M_Render_State state = {
        .line_idx = 0,
        .current_x = model->x,
        .current_y = model->y,
        .font_width = model->font_width,
        .font_height = model->font_height,
        .font_spacing = model->char_spacing,
        .char_buff_count = 0,
        .render_start_x = model->x
    };

    while (*text) {
        //! Handle newline character
        if (*text == '\n') {
            //! Print any remaining buffer before resetting
            handle_buffer(&state, config);

            //! Reset state for the next line and check for outbound ST7735_HEIGHT
            bool outbound_height = reset_render_state(&state);
            if (outbound_height) break;

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
                //! Print any remaining buffer before resetting
                handle_buffer(&state, config);

                //! Reset state for the next line and check for outbound ST7735_HEIGHT
                bool outbound_height = reset_render_state(&state);
                if (outbound_height) break;

            } else {
                //! Break the word and print the part that fits
                while (text < word_end) {
                    uint8_t char_width = font_width + char_spacing;

                    //! Handle wrapping for individual characters
                    if (state.current_x + char_width > ST7735_WIDTH) {
                        //! Print any remaining buffer before resetting
                        handle_buffer(&state, config);

                        //! Reset state for the next line and check for outbound ST7735_HEIGHT
                        bool outbound_height = reset_render_state(&state);
                        if (outbound_height) break;
                    }

                    //! Accumulate the current character
                    state.char_buff_data[state.char_buff_count++] = *text;

                    //! Check if the buffer is full
                    if (state.char_buff_count >= MAX_CHAR_COUNT) {
                        handle_buffer(&state, config);
                    }

                    //! Render the current character
                    uint16_t buff_len = model->font_width * model->font_height; // Number of pixels
                    uint16_t frame_buff[buff_len]; // Frame buffer for the character (uint16_t for RGB565)
                    draw_char(model, frame_buff, *text);
                    st7735_set_address_window(state.current_x, state.current_y, 
                        state.current_x + model->font_width - 1, state.current_y + model->font_height - 1, config);
                    mod_spi_data((uint8_t *)frame_buff, buff_len * 2, config); // Multiply by 2 for uint16_t size

                    //! Move to the next character position
                    state.current_x += char_width + char_spacing;       // Move x by character width + spacing
                    text++;                                             // Move to the next character
                }

                continue; // Skip the space handling below
            }
        }

        //! Render the current word
        while (text < word_end) {
            //! Accumulate the current character
            state.char_buff_data[state.char_buff_count++] = *text;

            //! Check if the buffer is full
            if (state.char_buff_count >= MAX_CHAR_COUNT) {
                handle_buffer(&state, config);
            }

            //! Render the current character
            uint16_t buff_len = model->font_width * model->font_height; // Number of pixels
            uint16_t frame_buff[buff_len]; // Frame buffer for the character (uint16_t for RGB565)
            draw_char(model, frame_buff, *text);
            st7735_set_address_window(state.current_x, state.current_y,
                state.current_x + model->font_width - 1, state.current_y + model->font_height - 1, config);
            mod_spi_data((uint8_t *)frame_buff, buff_len * 2, config); // Multiply by 2 for uint16_t size

            //! Move to the next character position
            state.current_x += font_width + char_spacing;     // Move x by character width + spacing
            text++;                                           // Move to the next character
        }

        //! Handle the space character
        if (*text == ' ') {
            //! Accumulate the space character
            state.char_buff_data[state.char_buff_count++] = *text;

            //! Check if the buffer is full
            if (state.char_buff_count >= MAX_CHAR_COUNT) {
                handle_buffer(&state, config);
            }

            //! Move to the next character position
            state.current_x += font_width + char_spacing; // Add space between words
            text++;                                       // Move to the next character
        }
    }

    //! Print the remaining buffer if it has data
    handle_buffer(&state, config);
}