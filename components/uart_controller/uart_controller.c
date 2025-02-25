#include "uart_controller.h"
#include <stdlib.h>
#include "driver/uart.h"

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUF_SIZE 1024

static uart_read_callback_t event_callback;
static char input_buffer[BUF_SIZE];
static int input_pos = 0;
static uint8_t* rx_buffer;

void uart_setup(uint8_t tx_pin, uint8_t rx_pin, uart_read_callback_t callback) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);

    rx_buffer = (uint8_t*) malloc(BUF_SIZE);
    event_callback = callback;
}


void uart_run(void) {
    // Check for console input
    int console_chars = uart_read_bytes(UART_NUM_0, (uint8_t*)&input_buffer[input_pos], 1, 0);
    if (console_chars > 0) {
        if (input_buffer[input_pos] == '\n') {
            // Enter pressed, send the input
            input_buffer[input_pos] = '\0';  // Null-terminate the string
            uart_write_bytes(UART_NUM_0, input_buffer, input_pos);
            uart_write_bytes(UART_NUM_0, "\n", 1);  // Add newline
            printf("Sent: %s\n", input_buffer);
            input_pos = 0;  // Reset buffer
        } else {
            input_pos++;
            if (input_pos >= BUF_SIZE - 1) {
                input_pos = 0;  // Prevent buffer overflow
            }
        }
    }

    // Check for received data on UART2
    int len = uart_read_bytes(UART_NUM_0, rx_buffer, BUF_SIZE, 0);
    if (len > 0) {
        rx_buffer[len] = 0;  // Null-terminate the received data
        printf("Received %d bytes: %s\n", len, (char*)rx_buffer);
        event_callback(rx_buffer, len);
    }
}