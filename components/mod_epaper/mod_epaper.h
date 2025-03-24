#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "mod_spi.h"



esp_err_t ssd1683_setup(uint8_t rst, uint8_t busy, M_Spi_Conf *conf);