#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <soc/gpio_num.h>

// Callback function
typedef void (*uart_read_callback_t)(uint8_t* data, size_t len);

// Function prototypes
void uart_setup(uart_read_callback_t callback);
void uart_setup_pin(uint8_t tx_pin, uint8_t rx_pin, uart_read_callback_t callback);
void uart_run(void);


void mod_uart_setup(int8_t tx_pin, int8_t rx_pin, uint8_t baud);
void mod_uart_task(void (*onReceive)(const char*, uint8_t));

#endif