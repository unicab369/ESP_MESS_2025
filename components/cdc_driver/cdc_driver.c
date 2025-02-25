#include <stdio.h>
#include <string.h>
#include "cdc_driver.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"

#define MAX_INPUT_SIZE 128
uint8_t data[MAX_INPUT_SIZE];
static char input_buffer[MAX_INPUT_SIZE]; // Static buffer to store concatenated input
static int input_index = 0;              // Index for the input buffer

void cdc_setup(void) {
    // Initialize USB CDC
    usb_serial_jtag_driver_config_t usb_cdc_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    esp_err_t ret = usb_serial_jtag_driver_install(&usb_cdc_config);
    if (ret != ESP_OK) {
        ESP_LOGE("USB_CDC", "Failed to install USB CDC driver");
        return;
    }
    ESP_LOGI("USB_CDC", "USB CDC driver initialized");
}

void clear_buffer() {
    input_index = 0;    // Reset buffer
    memset(input_buffer, 0, sizeof(input_buffer));
}

void cdc_read_task() {
    // Read data from USB CDC
    int length;
    length = usb_serial_jtag_read_bytes(data, sizeof(data) - 1, 0);
    if (length<=0) return;

    for (int i = 0; i < length; i++) {
        if (data[i] == '\n' || data[i] == '\r') {
            // Null-terminate the input buffer
            input_buffer[input_index] = '\0';
            ESP_LOGI("USB_CDC", "Entered data: %s", input_buffer);

            // Clear the input buffer for the next input
            clear_buffer();
        } else {
            // Append the character to the input buffer
            if (input_index < MAX_INPUT_SIZE - 1) {
                input_buffer[input_index++] = data[i];
                ESP_LOGI("USB_CDC", "string data: %s", input_buffer);
            } else {
                // Handle buffer overflow
                ESP_LOGE("USB_CDC", "Input buffer overflow!");
                clear_buffer();
            }
        }
    }
}