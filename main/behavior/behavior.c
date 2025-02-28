#include "behavior.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include "esp_log.h"
#include "littlefs/littlefs.h"
#include <ctype.h>

behavior_config_t device_behaviors[10];

void parse_hex_string(const char* input) {
    char str_copy[64]; // Make a copy of the input string (adjust size as needed)
    strncpy(str_copy, input, sizeof(str_copy) - 1);
    str_copy[sizeof(str_copy) - 1] = '\0'; // Ensure null termination

    const char* delimiter = ","; // Delimiter for tokenization
    char* token = strtok(str_copy, delimiter); // Tokenize the string

    while (token != NULL) {
        // Skip leading spaces
        while (isspace((unsigned char)*token)) {
            token++;
        }

        // Convert hex string to byte
        unsigned char byte = (unsigned char)strtol(token, NULL, 16);

        // Print the result (or store it in an array, etc.)
        printf("Hex: %s -> Byte: 0x%02X\n", token, byte);

        // Get the next token
        token = strtok(NULL, delimiter);
    }
}

void parse_mac_address(const char *mac_str, uint8_t mac[6]) {
    sscanf(mac_str, "%hhX-%hhX-%hhX-%hhX-%hhX-%hhX",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}


void littlefs_readfile_handler(char* file_name, char* buff, size_t len) {
    printf("%s", buff); // Print each line of the file
    printf("\n");
    parse_hex_string(buff);

    uint8_t mac[6];
    parse_mac_address(file_name, mac);
    printf("Parsed MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void behavior_setup(uint8_t* esp_mac) {
    littlefs_setup();
    littlefs_loadFiles("/littlefs/devices", littlefs_readfile_handler);
}

