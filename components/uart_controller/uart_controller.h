#ifndef UART_CONTROLLER_H
#define UART_CONTROLLER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <soc/gpio_num.h>

// Callback function
typedef void (*uart_read_callback_t)(uint8_t* data, size_t len);

// Function prototypes
void uart_setup(uint8_t tx_pin, uint8_t rx_pin, uart_read_callback_t callback);
void uart_run(void);

#endif // UART_CONTROLLER_H