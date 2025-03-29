#include "mod_spi.h"
#include "driver/gpio.h"

esp_err_t mod_spi_init(M_Spi_Conf *conf) {
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = conf->miso,
        .mosi_io_num = conf->mosi,
        .sclk_io_num = conf->clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(conf->host, &buscfg, SPI_DMA_CH2);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure SPI device for ST7735
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20*1000*1000 ,        //! Clock Speed
        .mode = 0,                              // SPI mode 0
        .queue_size = 1,
    };
    ret = spi_bus_add_device(conf->host, &devcfg, &conf->spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize GPIO pins
    if (conf->dc != -1) {
        gpio_set_direction(conf->dc, GPIO_MODE_OUTPUT);
    }
    
    return ret;
}

esp_err_t mod_spi_cmd(uint8_t cmd, M_Spi_Conf *conf) {
    spi_transaction_t t = {
        .length = 8, 
        .tx_buffer = &cmd,
    };

    if (conf->dc != -1) gpio_set_level(conf->dc, 0); // Command mode
    esp_err_t ret = spi_device_polling_transmit(conf->spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Send command Failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t mod_spi_data(uint8_t *data, uint16_t len, M_Spi_Conf *conf) {
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };

    if (conf->dc != -1) gpio_set_level(conf->dc, 1); // Data mode
    esp_err_t ret = spi_device_polling_transmit(conf->spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE("SPI", "Transmit data Failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t mod_spi_write_command(uint8_t cmd, const uint8_t *data, size_t len, M_Spi_Conf *conf) {
    spi_transaction_ext_t t = {
        .base = {
            .flags = 0,
            .cmd = cmd,
            .addr = 0,
            .length = len * 8, // Only data bits
            .tx_buffer = data,
        },
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
    };
    
    // Set DC low for command, then high for data automatically
    gpio_set_level(conf->dc, 0);
    esp_err_t ret = spi_device_polling_transmit(conf->spi_handle, &t.base);
    gpio_set_level(conf->dc, 1);
    
    return ret;
}