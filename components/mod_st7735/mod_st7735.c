#include "mod_st7735.h"

#include "esp_log.h"
#include "esp_err.h"
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

static const char *TAG = "MOD_ST7735";

#define DISPLAY_NUM_PIXELS (ST7735_WIDTH * ST7735_HEIGHT)

static uint16_t BACKGROUND_COLOR = 0x0000;

//# Set address window
esp_err_t st7735_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, M_Spi_Conf *conf) {           
    uint8_t data[] = {0x00, x0, 0x00, x1};
    //! ST7735_CASET
    esp_err_t ret = mod_spi_cmd(0x2A, conf);   
    ret = mod_spi_data(data, 4, conf);

    data[1] = y0;
    data[3] = y1;
    //! ST7735_RASET
    ret = mod_spi_cmd(0x2B, conf);
    ret = mod_spi_data(data, 4, conf);

    //! ST7735_RAMWR
    ret = mod_spi_cmd(0x2C, conf);

    return ret;
}

//# Fill screen
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

esp_err_t st7735_init(M_Spi_Conf *conf) {
    if (conf->rst != -1) {
        //! Set the RST pin
        gpio_set_direction(conf->rst, GPIO_MODE_OUTPUT);

        //! Reset the display
        gpio_set_level(conf->rst, 0);
        gpio_set_level(conf->rst, 1);
    }

    //! Set the DC pin
    if (conf->dc != -1) {
        gpio_set_direction(conf->dc, GPIO_MODE_OUTPUT);

        //! Send initialization commands
        mod_spi_cmd(0x01, conf); // Software reset
        vTaskDelay(pdMS_TO_TICKS(1));
        mod_spi_cmd(0x11, conf); // Sleep out
        
        //! Set color mode
        mod_spi_cmd(0x3A, conf);
        mod_spi_data((uint8_t[]){0x05}, 1, conf); // 16-bit color

        //! INVOFF: Disable display inversion
        mod_spi_cmd(0x20, conf); // 0x20 OFF | 0x21 ON
        
        //! Start display
        mod_spi_cmd(0x29, conf); // Display on

        //! Fill display color
        st7735_fill_screen(BACKGROUND_COLOR, conf);
    }

    return ESP_OK;
}


M_Render_State render_state = {
    .line_idx = 0,
    .char_count = 0
};

static void map_char_buffer(M_TFT_Text *model, uint16_t *my_buff, char c, uint8_t start_col, uint8_t total_cols) {
    //! Get the font data for the current character
    const uint8_t *char_data = &model->font[(c - 32) * model->font_width * ((model->font_height + 7) / 8)];

    //! Render the character into the frame buffer
    for (int j = 0; j < model->font_height; j++) { // Rows
        for (int i = 0; i < model->font_width; i++) { // Columns
            // Calculate the index in the flat buffer
            int index = (start_col + i) + j * total_cols;

            if (char_data[(i + (j / 8) * model->font_width)] & (1 << (j % 8))) {
                //! Pixel is part of the character (foreground color)
                my_buff[index] |= model->color; // RGB565 color
            } else {
                //! Pixel is not part of the character (background color)
                my_buff[index] |= BACKGROUND_COLOR; // RGB565 background color
            }
        }
    }
}


//# Draw Pixel
esp_err_t st7735_draw_pixel(uint16_t color, M_Spi_Conf *conf) {
    uint8_t data[] = { color >> 8, color & 0xFF };
    return mod_spi_data(data, 2, conf);
}


static void send_text_buffer(M_TFT_Text *model, M_Spi_Conf *config) {
    if (render_state.char_count <= 0) return;

    uint16_t spacing = model->char_spacing;
    uint8_t char_width = model->font_width + spacing;
    uint8_t font_height = model->font_height;

    uint16_t arr_width = char_width * render_state.char_count;
    uint16_t buff_len = arr_width * font_height;
    uint16_t frame_buff[buff_len];

    //! Initialize buffer to 0 (background color)
    memset(frame_buff, 0, sizeof(frame_buff));

    //! Render each character into the buffer
    for (uint8_t i = 0; i < render_state.char_count; i++) {
        // Calculate the starting column for the current character
        uint8_t start_col = i * char_width;
        map_char_buffer(model, frame_buff, render_state.char_buff[i], start_col, arr_width);
    }

    #if LOG_BUFFER_CONTENT
        //# Print the buffer
        ESP_LOGI(TAG, "Frame Buffer Contents:");

        for (int j = 0; j < font_height; j++) {
            for (int i = 0; i < arr_width; i++) {
                uint16_t value = frame_buff[i + j * arr_width];
                
                if (value < 1) {
                    printf("__");
                } else {
                    printf("##");
                    // printf("%04X ", value);
                }
            }
            printf("\n");
        }
    #endif

    uint8_t x1 = render_state.x0 + (render_state.char_count * model->font_width) + (render_state.char_count - 1) * spacing;
    uint8_t y1 = render_state.current_y + font_height - 1;

    //# Print the buffer details
    printf("\nL%d: %d char, spacing: %u,\t [x=%d, y=%d] to [x=%d, y=%d]",
        render_state.line_idx, render_state.char_count, spacing,
        // start position
        render_state.x0, render_state.current_y, 
        // end position
        x1, y1
    );

    // Print the accumulated characters
    printf("\t\t");
    for (uint8_t i = 0; i < render_state.char_count; i++) {
        printf("%c", render_state.char_buff[i]);
    }

    //! Send the buffer to the display    
    st7735_set_address_window(render_state.x0, render_state.current_y, x1, y1, config);
    mod_spi_data((uint8_t *)frame_buff, buff_len * 2, config); // Multiply by 2 for uint16_t size

    // Reset the accumulated character count and update render_start_x
    render_state.char_count = 0;
    render_state.x0 = x1;
}

bool flush_buffer_and_reset(uint8_t font_height) {
    //! Move to the next line
    render_state.line_idx++;

    //! Reset state for the next line
    render_state.x0 = 0;                                    // Reset render_start_x to the start position
    render_state.current_y += font_height + 1; // Move y to the next line

    //! Check if current_y is out of bounds
    return render_state.current_y + font_height > ST7735_HEIGHT;
}

void st7735_draw_text(M_TFT_Text *model, M_Spi_Conf *config) {
    if (!model || !config || !model->text || !model->font) return; // Error handling

    // Pointer to the text
    const char *text = model->text;
    uint8_t char_width = model->font_width + model->char_spacing;
    uint8_t font_height = model->font_height;

    //# Initialize rendering state
    uint8_t current_x = model->x;

    render_state.current_y = model->y;
    render_state.x0 = model->x;

    while (*text) {
        //# Find the end of the current word and calculate its width
        uint8_t word_width = 0;

        const char *word_end = text;
        while (*word_end && *word_end != ' ' && *word_end != '\n') {
            word_width += char_width;
            word_end++;
        }

        //# Handle newline character
        if (*text == '\n') {
            current_x = 0;

            send_text_buffer(model, config);
            if (flush_buffer_and_reset(font_height)) break;        //! Exit the outerloop if break
            text++;     // Move to the next character
            continue;   // continue next iteration
        }

        if (current_x + word_width > ST7735_WIDTH) {
            //# Handle page wrapping
            if (model->page_wrap == 0) {
                //! Stop printing and move to the next line
                current_x = 0;

                send_text_buffer(model, config);
                if (flush_buffer_and_reset(font_height)) break;

                //! Skip all remaining characters on the same line
                while (*text && *text != '\n') text++;
                continue; // Skip the rest of the loop and start processing the next line
            }
            //# Handle word wrapping
            else if (model->word_wrap) {
                //! Stop printing and move to the next line
                current_x = 0;

                send_text_buffer(model, config);
                if (flush_buffer_and_reset(font_height)) break;
            }
        }

        //# This while loop processes a sequence of characters (stored in the text pointer) until it reaches
        //# the end of a word (word_end) or encounters a space character (' ')
        
        while (text < word_end || (*text == ' ' && text < word_end + 1)) {
            //# Handle word breaking when word_wrap is disabled
            if (!model->word_wrap && current_x + char_width > ST7735_WIDTH) {
                current_x = 0;

                send_text_buffer(model, config);
                if (flush_buffer_and_reset(font_height)) break;
            }

            // Accumulate the current character
            render_state.char_buff[render_state.char_count++] = *text;

            //! Check if the buffer is full
            if (render_state.char_count >= MAX_CHAR_COUNT) {
                send_text_buffer(model, config);
            }

            // Move to the next character position
            current_x += char_width;     // Move x by character width + spacing
            text++;                                                 // Move to the next character
        }
    }

    //! Print the remaining buffer if it has data
    send_text_buffer(model, config);
    ESP_LOGI(TAG, "Finish drawing text");
}