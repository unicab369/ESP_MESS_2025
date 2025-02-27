#ifndef ESPNOW_CONTROLLER_H
#define ESPNOW_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>
#include <esp_err.h> 

// Function prototypes
void espnow_controller_setup(void);
void espnow_controller_send(void);

#endif