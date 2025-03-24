#include "mod_epaper.h"


#define EPD_WIDTH  250
#define EPD_HEIGHT 122


#define EPD_BUFFER_SIZE 500

static uint8_t busy_pin;


static bool epd_is_busy() {
    return gpio_get_level(busy_pin);
}

static void epd_wait_until_idle() {
    while(epd_is_busy()) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}



void ssd1683_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h, M_Spi_Conf *conf) {
    mod_spi_cmd(0x91, conf);  // PARTIAL_IN
    mod_spi_cmd(0x90, conf);  // PARTIAL_WINDOW
    
    // Set boundaries
    uint8_t data[1] = { 0x01 };
    data[0] = x >> 8;
    mod_spi_data(data, 1, conf);     // x-start high

    data[0] = x & 0xff;
    mod_spi_data(data, 1, conf);   // x-start low

    data[0] = (x+w-1) >> 8;
    mod_spi_data(data, 1, conf);   // x-end high

    data[0] = (x+w-1) & 0xff;
    mod_spi_data(data, 1, conf); // x-end low
    
    data[0] = y >> 8;
    mod_spi_data(data, 1, conf);     // y-start high

    data[0] = y & 0xff;
    mod_spi_data(data, 1, conf);   // y-start low

    data[0] = (y+h-1) >> 8;
    mod_spi_data(data, 1, conf);   // y-end high

    data[0] = (y+h-1) & 0xff;
    mod_spi_data(data, 1, conf); // y-end low

    data[0] = 0x01;
    mod_spi_data(data, 1, conf);
}

uint8_t epd_framebuffer[500];

void epd_update_partial(uint16_t x, uint16_t y, uint16_t w, uint16_t h, M_Spi_Conf *conf) {
    // 1. Set partial window
    ssd1683_set_window(x, y, w, h, conf);
    
    // 2. Calculate affected bytes
    uint16_t start_col = x / 8;
    uint16_t end_col = (x + w - 1) / 8;
    uint16_t cols = end_col - start_col + 1;
    
    // 3. Send data only for affected rows
    mod_spi_cmd(0x10, conf);  // DATA_START_TRANSMISSION_1 (black)
    for (uint16_t row = y; row < y + h; row++) {
        uint32_t row_start = (row * EPD_WIDTH / 8) + start_col;
        for (uint16_t col = 0; col < cols; col++) {
            uint8_t data = epd_framebuffer[row_start + col]; 
            mod_spi_data(&data, 1, conf);
        }
    }
    
    // 4. Refresh display (partial)
    mod_spi_cmd(0x12, conf);  // DISPLAY_REFRESH
    epd_wait_until_idle();
    
    // 5. Return to normal mode
    mod_spi_cmd(0x92, conf);  // PARTIAL_OUT
}

// void ssd1683_update_display(M_Spi_Conf *conf) {
//     mod_spi_cmd(0x12, conf);        // DISPLAY_REFRESH
//     epd_wait_until_idle();
// }


void ssd1683_update_display(M_Spi_Conf *conf) {
    // Send black/white data (for displays that need both)
    mod_spi_cmd(0x10, conf);  // DATA_START_TRANSMISSION_1 (black)

    for (uint32_t i = 0; i < EPD_BUFFER_SIZE; i++) {
        uint8_t data = epd_framebuffer[i];
        mod_spi_data(&data, 1, conf);
    }
    
    // Some displays need red data too (if you have a 3-color display)
    // epd_write_command(0x13);  // DATA_START_TRANSMISSION_2 (red)
    // for (uint32_t i = 0; i < EPD_BUFFER_SIZE; i++) {
    //     epd_write_data(0x00);  // Typically all zeros for no red
    // }
    
    // Refresh the display
    mod_spi_cmd(0x12, conf);  // DISPLAY_REFRESH
    epd_wait_until_idle();
}

void ssd1683_sleep(M_Spi_Conf *conf) {
    mod_spi_cmd(0x02, conf);        // POWER_OFF
    epd_wait_until_idle();

    mod_spi_cmd(0x07, conf);        // DEEP_SLEEP
    uint8_t data[1] = { 0xFF };
    mod_spi_data(data, 1, conf);
}

void ssd1683_clear(M_Spi_Conf *conf) {
    uint16_t width = 250;
    uint16_t height = 122;
    uint16_t bytes_per_line = (width + 7) / 8;
    
    mod_spi_cmd(0x10, conf); // DATA_START_TRANSMISSION_1

    for(uint16_t i = 0; i < height; i++) {
        for(uint16_t j = 0; j < bytes_per_line; j++) {
            uint8_t data[1] = { 0xFF };
            mod_spi_data(data, 1, conf); // White
        }
    }
    
    mod_spi_cmd(0x13, conf); // DATA_START_TRANSMISSION_2

    for(uint16_t i = 0; i < height; i++) {
        for(uint16_t j = 0; j < bytes_per_line; j++) {
            mod_spi_data(0x00, 1, conf); // Black
        }
    }
    
    ssd1683_update_display(conf);
}


void epd_draw_hline_fast(int x, int y, int w, uint8_t color, M_Spi_Conf *conf) {
    if (y < 0 || y >= EPD_HEIGHT) return;
    
    int x_end = x + w;
    if (x_end > EPD_WIDTH) x_end = EPD_WIDTH;
    if (x < 0) x = 0;
    
    // Process full bytes first
    int full_bytes_start = (x + 7) / 8;  // First fully covered byte
    int full_bytes_end = x_end / 8;      // Last fully covered byte
    
    // Partial byte at start
    if (x % 8 != 0) {
        uint32_t byte_pos = (x + y * EPD_WIDTH) / 8;
        uint8_t mask = 0xFF << (8 - (x % 8));

        if (color == 0) {  // Black
            epd_framebuffer[byte_pos] &= ~mask;
        } else {  // White
            epd_framebuffer[byte_pos] |= mask;
        }
        x = full_bytes_start * 8;
    }
    
    // Full bytes
    if (full_bytes_end > full_bytes_start) {
        uint32_t byte_pos = (x + y * EPD_WIDTH) / 8;
        uint8_t val = (color == 0) ? 0x00 : 0xFF;
        memset(&epd_framebuffer[byte_pos], val, full_bytes_end - full_bytes_start);
        x = full_bytes_end * 8;
    }
    
    // Partial byte at end
    if (x < x_end) {
        uint32_t byte_pos = (x + y * EPD_WIDTH) / 8;
        uint8_t mask = 0xFF >> (x % 8);

        if (color == 0) {  // Black
            epd_framebuffer[byte_pos] &= ~mask;
        } else {  // White
            epd_framebuffer[byte_pos] |= mask;
        }
    }

    // Update display
    ssd1683_update_display(conf);
}

esp_err_t ssd1683_setup(uint8_t rst, uint8_t busy, M_Spi_Conf *conf) {
    busy_pin = busy;

    //! Configure GPIOs
    gpio_set_direction(rst, GPIO_MODE_OUTPUT);
    gpio_set_direction(busy, GPIO_MODE_INPUT);


    //! Set the RST pin
    gpio_set_level(rst, 0);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(rst, 1);
    vTaskDelay(200 / portTICK_PERIOD_MS);


    uint8_t data[4] = { 
        0x03,   // VDS_EN, VDG_EN
        0x00,   // VCOM_HV, VGHL_LV
        0x2b,   // VDH
        0x2b    // VDL
    };
    mod_spi_write_command(0x01, data, sizeof(data), conf);     // POWER SETTING

    // epd_write_command(0x01); // POWER SETTING
    // epd_write_data(0x03);    // VDS_EN, VDG_EN
    // epd_write_data(0x00);    // VCOM_HV, VGHL_LV
    // epd_write_data(0x2b);    // VDH
    // epd_write_data(0x2b);    // VDL
    
    data[0] = 0x17;
    data[1] = 0x17;
    data[2] = 0x17;
    mod_spi_write_command(0x06, data, 3, conf);                 // BOOSTER SOFT START

    // epd_write_command(0x06); // BOOSTER SOFT START
    // epd_write_data(0x17);
    // epd_write_data(0x17);
    // epd_write_data(0x17);
    
    mod_spi_cmd(0x04, conf);          // POWER ON
    epd_wait_until_idle();

    data[0] = 0x3c;
    mod_spi_write_command(0x30, data, 1, conf);     // PLL CONTROL
    
    // epd_write_command(0x30); // PLL CONTROL
    // epd_write_data(0x3c);    // 3A 100HZ   29 150Hz 39 200HZ  31 171HZ
    
    data[0] = 0XFA;
    data[1] = 0x01;                 // 250
    data[2] = 0x00;                 // 122
    mod_spi_write_command(0x61, data, 3, conf);    // RESOLUTION SETTING
    
    // epd_write_command(0x61); // RESOLUTION SETTING
    // epd_write_data(0xfa);    // 250
    // epd_write_data(0x01);    // 122
    // epd_write_data(0x00);
    
    data[0] = 0x12;
    mod_spi_write_command(0x82, data, 1, conf);

    // epd_write_command(0x82); // VCM_DC_SETTING
    // epd_write_data(0x12);
    
    data[0] = 0x97;
    mod_spi_write_command(0x50, data, 1, conf);

    // epd_write_command(0x50); // VCOM AND DATA INTERVAL SETTING
    // epd_write_data(0x97);

    ssd1683_clear(conf);

    // epd_draw_hline_fast(0, 0, 20, 0xFF, conf);
    epd_draw_hline_fast(0, 20, 30, 0xFF, conf);
    // epd_draw_hline_fast(0, 30, 40, 0xFF, conf);

    // ssd1683_sleep(conf);

    return ESP_OK;
}


