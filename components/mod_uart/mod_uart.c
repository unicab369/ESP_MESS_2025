#include "mod_uart.h"
#include <stdlib.h>
#include "driver/uart.h"

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define BUF_SIZE 128
#define INVALID_PIN 255

static uart_read_callback_t event_callback;
static char input_buffer[BUF_SIZE];
static int input_pos = 0;
static uint8_t* rx_buffer;


void mod_uart_setup(int8_t tx_pin, int8_t rx_pin, uint8_t baud) {
    if (tx_pin < 0 || rx_pin < 0) return;

    uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Install UART driver with RX buffer
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, BUF_SIZE, 0, 0, NULL, 0));
}

static char uart_data[BUF_SIZE];

void mod_uart_task(void (*onReceive)(const char*, uint8_t)) {
    // Read data from UART
    uint8_t len = uart_read_bytes(UART_NUM_0, uart_data, BUF_SIZE, 1);
    
    if (len > 0) {
        // Print raw NMEA sentences (e.g., "$GPGGA,...")
        printf("Received: %.*s\n", len, uart_data);
        
        onReceive(uart_data, len);
        // Parse specific NMEA messages here (see Step 3)
    }
}

void uart_setup(uart_read_callback_t callback) {
    uart_setup_pin(INVALID_PIN, INVALID_PIN, callback);
}

void uart_setup_pin(uint8_t tx_pin, uint8_t rx_pin, uart_read_callback_t callback) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_NUM_0, &uart_config);

    if (tx_pin != INVALID_PIN && rx_pin != INVALID_PIN) {
        uart_set_pin(UART_NUM_0, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    } else {
        uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    
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