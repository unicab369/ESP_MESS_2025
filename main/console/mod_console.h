#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


/**
 * @brief Initialize console peripheral type
 *
 * Console peripheral is based on sdkconfig settings
 *
 * UART                 CONFIG_ESP_CONSOLE_UART_DEFAULT
 * USB_OTG              CONFIG_ESP_CONSOLE_USB_CDC
 * USB_SERIAL_JTAG      CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
 */
void mod_console_setup(
    int port, int8_t tx_pin, int8_t rx_pin,
    int8_t baud, const char* history_path
);

/**
 * @brief Initialize console prompt
 *
 * This function adds color code to the prompt (if the console supports escape sequences)
 *
 * @param prompt_str Prompt in form of string eg esp32s3>
 *
 * @return
 *     - pointer to initialized prompt
 */
char *setup_prompt(const char *prompt_str);
