#include "driver/i2c.h"

typedef struct {
    i2c_port_t port;
    uint8_t address;
} M_I2C_Device;

esp_err_t i2c_setup(uint8_t scl_pin, uint8_t sda_pin);

M_I2C_Device* i2c_device_create(i2c_port_t port, uint8_t address);
esp_err_t i2c_device_delete(M_I2C_Device* device);

esp_err_t i2c_write_byte_slow(const M_I2C_Device* device, uint8_t byte);
esp_err_t i2c_read_bytes_slow(const M_I2C_Device* device, uint8_t *output, size_t len);

esp_err_t i2c_read(const M_I2C_Device* device, uint8_t *output, size_t len);
esp_err_t i2c_write(const M_I2C_Device* device, const uint8_t *buffer, size_t len);

esp_err_t i2c_write_register_byte(const M_I2C_Device* device, uint8_t cmd, uint8_t value);

esp_err_t i2c_write_read_register(
    const M_I2C_Device *device, uint8_t cmd,
    uint8_t *output_buff, uint8_t len
);

esp_err_t i2c_write_register(
    const M_I2C_Device* device, uint8_t cmd,
    const uint8_t *data, size_t len
);
