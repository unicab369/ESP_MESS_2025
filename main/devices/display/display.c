#include "display.h"

#include "mod_ssd1306.h"
#include "driver/i2c.h"

#include "esp_log.h"
#include "esp_err.h"
#include "mod_i2c.h"

static const char *TAG = "I2C_DEVICE";


void display_setup(uint8_t scl_pin, uint8_t sda_pin) {
    esp_err_t ret = i2c_setup(scl_pin, sda_pin);

    ssd1306_setup(0x3C);
    ssd1306_print_str("Hello Bee", 0);
}

void display_spi_setup(uint8_t rst, M_Spi_Conf *conf) {
    st7735_init(rst, conf);
    st7735_draw_text(0, 0, "Hello Bee! What is Thy bidding, my master?", 0x00AA, true, conf);
}

void display_print_str(const char *str, uint8_t line) {
    ssd1306_print_str(str, line);
}
