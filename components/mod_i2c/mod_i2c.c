#include "mod_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

esp_err_t i2c_setup(uint8_t scl_pin, uint8_t sda_pin) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
    if (err != ESP_OK) return err;
    err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    return err;
}

i2c_device_t* i2c_device_create(i2c_port_t port, const uint8_t address) {
    i2c_device_t *device = (i2c_device_t *) calloc(1, sizeof(i2c_device_t));
    device->port = port;
    device->address = address;
    return device;
}

esp_err_t i2c_device_delete(i2c_device_t* device) {
    free(device);
    return ESP_OK;
}

esp_err_t i2c_read_bytes_slow(const i2c_device_t* device, uint8_t *output, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret = i2c_master_start(cmd);
    if (ret != ESP_OK) return ret;

    ret = i2c_master_write_byte(cmd, (device->address << 1) | I2C_MASTER_READ, true);
    if (ret != ESP_OK) return ret;

    for (int i=0; i<len; i++) {
        ret = i2c_master_read_byte(cmd, &output[i], I2C_MASTER_ACK);
        if (ret != ESP_OK) return ret;
    }

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK) return ret;
    ret = i2c_master_cmd_begin(device->port, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t i2c_write_byte_slow(const i2c_device_t* device, const uint8_t byte) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    esp_err_t ret = i2c_master_start(cmd);
    if (ret != ESP_OK) return ret;

    ret = i2c_master_write_byte(cmd, (device->address << 1) | I2C_MASTER_WRITE, true);
    if (ret != ESP_OK) return ret;
    ret = i2c_master_write_byte(cmd, byte, true);
    if (ret != ESP_OK) return ret;

    ret = i2c_master_stop(cmd);
    if (ret != ESP_OK) return ret;
    ret = i2c_master_cmd_begin(device->port, cmd, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t i2c_write_fast(const i2c_device_t* device, const uint8_t *buffer, size_t len) {
    // printf("*****IM HERE 222 val1: %d, val2: %d, len: %d\n", buffer[0], buffer[1], len);
    // printf("****PORT: %d, addr: %d\n", device->port, device->address);

    return i2c_master_write_to_device(device->port, device->address, buffer, len, pdMS_TO_TICKS(50));
}

esp_err_t i2c_read_fast(const i2c_device_t* device, uint8_t *output, size_t len) {
    return i2c_master_read_from_device(device->port, device->address, output, len, pdMS_TO_TICKS(50));
}

esp_err_t i2c_write_command(const i2c_device_t* device, uint8_t cmd, uint8_t value) {
    uint8_t buff[2] = {cmd, value};
    return i2c_write_fast(device, buff, sizeof(buff));
}

esp_err_t i2c_write_command_data(
    const i2c_device_t* device, uint8_t cmd,
    const uint8_t *data, size_t len
) {
    uint8_t *buff = malloc(len + 1);
    buff[0] = cmd;
    memcpy(buff + 1, data, len);
    esp_err_t ret = i2c_write_fast(device, buff, len + 1);
    free(buff);
    return ret;
}