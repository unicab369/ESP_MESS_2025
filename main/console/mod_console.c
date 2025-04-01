/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "driver/uart_vfs.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_vfs_cdcacm.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"


#define CONSOLE_MAX_CMDLINE_ARGS 8
#define CONSOLE_MAX_CMDLINE_LENGTH 256
#define CONSOLE_PROMPT_MAX_LEN (32)

char prompt[CONSOLE_PROMPT_MAX_LEN]; // Prompt to be printed before each line

void mod_console_setup(
    int port, int8_t tx_pin, int8_t rx_pin,
    int8_t baud, const char* history_path
) {
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    if (tx_pin > 0 && rx_pin > 0) {
        printf("mod_console uart mode\n");

        /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
        uart_vfs_dev_port_set_rx_line_endings(port, ESP_LINE_ENDINGS_CR);
        /* Move the caret to the beginning of the next line on '\n' */
        uart_vfs_dev_port_set_tx_line_endings(port, ESP_LINE_ENDINGS_CRLF);

        /* Configure UART. Note that REF_TICK is used so that the baud rate remains
        * correct while APB frequency is changing in light sleep mode. */
        const uart_config_t uart_config = {
                .baud_rate = baud,
                .data_bits = UART_DATA_8_BITS,
                .parity = UART_PARITY_DISABLE,
                .stop_bits = UART_STOP_BITS_1,
            #if SOC_UART_SUPPORT_REF_TICK
                .source_clk = UART_SCLK_REF_TICK,
            #elif SOC_UART_SUPPORT_XTAL_CLK
                .source_clk = UART_SCLK_XTAL,
            #endif
        };

        uart_set_pin(UART_NUM_0, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
        
        /* Install UART driver for interrupt-driven reads and writes */
        ESP_ERROR_CHECK( uart_driver_install(port, 256, 0, 0, NULL, 0) );
        ESP_ERROR_CHECK( uart_param_config(port, &uart_config) );

        /* Tell VFS to use UART driver */
        uart_vfs_dev_use_driver(port);
    } else {
        #if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
            printf("mod_console USB serial Jtag mode\n");

            /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
            usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
            /* Move the caret to the beginning of the next line on '\n' */
            usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

            /* Enable blocking mode on stdin and stdout */
            fcntl(fileno(stdout), F_SETFL, 0);
            fcntl(fileno(stdin), F_SETFL, 0);

            usb_serial_jtag_driver_config_t jtag_config = {
                .tx_buffer_size = 256,
                .rx_buffer_size = 256,
            };

            /* Install USB-SERIAL-JTAG driver for interrupt-driven reads and writes */
            ESP_ERROR_CHECK( usb_serial_jtag_driver_install(&jtag_config));

            /* Tell vfs to use usb-serial-jtag driver */  
            usb_serial_jtag_vfs_use_driver();

        #else
            printf("mod_console CDC mode\n");

            // // #if defined(CONFIG_ESP_CONSOLE_USB_CDC)
            // /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
            // esp_vfs_dev_cdcacm_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
            // /* Move the caret to the beginning of the next line on '\n' */
            // esp_vfs_dev_cdcacm_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

            // /* Enable blocking mode on stdin and stdout */
            // fcntl(fileno(stdout), F_SETFL, 0);
            // fcntl(fileno(stdin), F_SETFL, 0);
        #endif
    }

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_args = CONSOLE_MAX_CMDLINE_ARGS,
        .max_cmdline_length = CONSOLE_MAX_CMDLINE_LENGTH,
    #if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
    #endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within single line. */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);
    linenoiseHistorySetMaxLen(100);     /* Set command history size */

     /* Set command maximum length */
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);     
    linenoiseAllowEmpty(false); /* Don't return empty lines */

    #if CONFIG_CONSOLE_STORE_HISTORY
        /* Load command history from filesystem */
        linenoiseHistoryLoad(history_path);
    #endif // CONFIG_CONSOLE_STORE_HISTORY

    /* Figure out if the terminal supports escape sequences zero indicates success */
    const int probe_status = linenoiseProbe();
    if (probe_status) linenoiseSetDumbMode(1);
}


char *setup_prompt(const char *prompt_str) {
    const char *prompt_temp = "esp>";
    if (prompt_str) prompt_temp = prompt_str;
    snprintf(prompt, CONSOLE_PROMPT_MAX_LEN - 1, LOG_COLOR_I "%s " LOG_RESET_COLOR, prompt_temp);

    if (linenoiseIsDumbMode()) {
        #if CONFIG_LOG_COLORS
            /* Since the terminal doesn't support escape sequences,
            * don't use color codes in the s_prompt. */
            snprintf(prompt, CONSOLE_PROMPT_MAX_LEN - 1, "%s ", prompt_temp);
        #endif //CONFIG_LOG_COLORS
    }
    return prompt;
}
