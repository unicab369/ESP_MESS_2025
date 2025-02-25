#ifndef ESPNOW_DIRVER_H
#define ESPNOW_DIRVER_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>
#include <esp_err.h> 

// Function prototypes
esp_err_t espNow_setup(void);
esp_err_t espnow_send(void);

#endif