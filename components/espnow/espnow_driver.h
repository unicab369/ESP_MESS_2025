#ifndef ESPNOW_DIRVER_H
#define ESPNOW_DIRVER_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>
#include <esp_err.h> 

typedef void (*espnow_message_cb)(const uint8_t *data, int len);

// Function prototypes
esp_err_t espnow_setup(espnow_message_cb callback);
esp_err_t espnow_send(uint8_t* data, size_t len);

#endif