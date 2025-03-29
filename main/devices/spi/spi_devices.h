#include <unistd.h>
#include <stdint.h>

void spi_devices_setup(
    uint8_t host, 
    int8_t mosi_pin, int8_t miso_pin, int8_t clk_pin, int8_t cs_pin,
    int8_t dc_pin, int8_t rst_pin,
    int8_t cs_x0, int8_t cs_x1
);