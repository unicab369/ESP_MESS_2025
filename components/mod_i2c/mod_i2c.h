#include "driver/i2c.h"

typedef struct {
    i2c_port_t port;
    uint8_t address;
} i2c_device_t;

esp_err_t i2c_setup(uint8_t scl_pin, uint8_t sda_pin);

i2c_device_t* i2c_device_create(i2c_port_t port, const uint16_t address);
esp_err_t i2c_device_delete(i2c_device_t* device);

esp_err_t i2c_write_byte_slow(const i2c_device_t* device, const uint8_t byte);
esp_err_t i2c_read_bytes_slow(const i2c_device_t* device, uint8_t *output, size_t len);