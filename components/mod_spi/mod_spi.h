#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

#include "driver/spi_master.h"

typedef struct {
    spi_device_handle_t spi_handle;
    spi_host_device_t host;
    int8_t mosi;
    int8_t miso;
    int8_t clk;
    int8_t dc;
    esp_err_t err;
} M_Spi_Conf;

esp_err_t mod_spi_init(M_Spi_Conf *conf);
esp_err_t mod_spi_cmd(uint8_t cmd, M_Spi_Conf *conf);
esp_err_t mod_spi_data(uint8_t *data, uint16_t len, M_Spi_Conf *conf);