/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
// #include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/soc_caps.h"
#include "cmd_wifi/cmd_wifi.h"
#include "cmd_nvs/cmd_nvs.h"
#include "base_console.h"
#include "cmd_system/cmd_system.h"
#include "driver/uart.h"

static const char *TAG = "APP_CONSOLE";
static char *prompt;

#if CONFIG_CONSOLE_STORE_HISTORY
    #define MOUNT_PATH "/data"
    #define HISTORY_PATH MOUNT_PATH "/history.txt"

    static void initialize_filesystem(void)
    {
        static wl_handle_t wl_handle;
        const esp_vfs_fat_mount_config_t mount_config = {
                .max_files = 4,
                .format_if_mount_failed = true
        };
        esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
            return;
        }
    }
    #else
    #define HISTORY_PATH NULL
#endif // CONFIG_CONSOLE_STORE_HISTORY


void app_console_setup(void)
{
    #if CONFIG_CONSOLE_STORE_HISTORY
        initialize_filesystem();
        ESP_LOGI(TAG, "Command history enabled");
    #else
        ESP_LOGI(TAG, "Command history disabled");
    #endif
    
    initialize_console_peripheral();            /* Initialize console output periheral (UART, USB_OTG, USB_JTAG) */
    initialize_console_library(HISTORY_PATH);   /* Initialize linenoise library and esp_console*/

    cmd_system_register();

    #if (CONFIG_ESP_WIFI_ENABLED || CONFIG_ESP_HOST_WIFI_ENABLED)
        register_wifi();
    #endif
    
    register_nvs();

    printf("\nType 'help' to get the list of commands.\n"
        "Use UP/DOWN arrows to navigate through command history.\n"
        "Press TAB when typing command name to auto-complete.\n"
        "Ctrl+C will terminate the console environment.\n");

    if (linenoiseIsDumbMode()) {
        printf("\nYour terminal application does not support escape sequences.\n"
            "Line editing and history features are disabled.\n"
            "On Windows, try using Putty instead.\n");
    }
}

void app_console_deinit(void) {
    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
}

#define BUF_SIZE 256
static char input_buffer[BUF_SIZE];
static int input_index = 0;

static void handle_line() {
    /* Try to run the command */
    int ret;
    esp_err_t err = esp_console_run(input_buffer, &ret);
    if (err == ESP_ERR_NOT_FOUND) {
        printf("Unrecognized command\n");
    } else if (err == ESP_ERR_INVALID_ARG) {
        // command was empty
    } else if (err == ESP_OK && ret != ESP_OK) {
        printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
    } else if (err != ESP_OK) {
        printf("Internal error: %s\n", esp_err_to_name(err));
    }
}

void app_console_task(void) {
    // Check for console input
    char c;
    int len = uart_read_bytes(UART_NUM_0, (uint8_t*)&c, 1, 0);
    if (len <= 0) return;

    // Handle escape sequences (e.g., arrow keys)
    if (c == '\x1b') {  // Escape character
        // Read the next two characters to determine if it's an arrow key
        char seq[2];
        int seq_len = uart_read_bytes(UART_NUM_0, (uint8_t*)seq, 2, 0);

        // Check for arrow keys. seq[1] == 'C' for left arrow - permitable
        bool check_keys = seq[1] == 'A' || seq[1] == 'B' || seq[1] == 'D';
        if (seq_len == 2 && seq[0] == '[' && check_keys) {
            // Ignore arrow keys (up, down, left, right)
            return;
        }
    }

    // Handle Enter key
    if (c == '\n' || c == '\r') {
        input_buffer[input_index] = '\0';  // Null-terminate
        printf("\n\n");    // make new line
        handle_line();
        input_index = 0;
    }
    // Handle Backspace (ASCII 8) or Delete (ASCII 127)
    else if (c == 8 || c == 127) {
        if (input_index > 0) {
            input_index--;
            // Erase from terminal: move back, overwrite with space, move back again
            uart_write_bytes(UART_NUM_0, "\b \b", 3);
        }
    }
    // Handle regular characters
    else {
        if (input_index < BUF_SIZE - 1) {  // Leave space for null terminator
            input_buffer[input_index] = c;
            input_index++;
            // Echo character back
            uart_write_bytes(UART_NUM_0, &c, 1);
        }
        else {
            // Buffer full - you could add a beep or visual feedback
            uart_write_bytes(UART_NUM_0, "\a", 1);  // Terminal bell
        }
    }
}