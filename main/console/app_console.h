#ifndef APP_CONSOLE_H
#define APP_CONSOLE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


void app_console_setup(uint8_t port, int8_t tx_pin, int8_t rx_pin, uint8_t baud);
void app_console_deinit(void);
void app_console_task(void);

#endif