#include "behavior.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "esp_log.h"
#include "littlefs/littlefs.h"
#include <ctype.h>

#define BEHAVIOR_VALIDATION_CODE 0x33
#define INDEX_VALIDATION 0
#define INDEX_INPUT_CMD 1
#define INDEX_INPUT_GPIO 2
#define INDEX_OUTPUT_CMD 17
#define PARSE_STRING_LEN 128

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

static const char *TAG = "BEHAVIOR";

static behavior_config_t device_behaviors[10];
static uint8_t behaviors_index = 0;
static uint8_t device_mac[6];
static behavior_output_cb output_callback;


void print_bytes(uint8_t* data, size_t len) {
    // Print all bytes in hexadecimal format
    printf("Print Bytes:\n");
    for (size_t i = 0; i < len; i++) {
        printf("0x%02X ", data[i]);
        if ((i + 1) % 8 == 0) { // Print 8 bytes per line for readability
            printf("\n");
        }
    }
    printf("\n");
}


static void parse_hex_string(const char* input, uint8_t* output) {
    char str_copy[PARSE_STRING_LEN]; // Make a copy of the input string (adjust size as needed)
    strncpy(str_copy, input, sizeof(str_copy) - 1);
    str_copy[sizeof(str_copy) - 1] = '\0'; // Ensure null termination

    const char* delimiter = ",";
    char* token = strtok(str_copy, delimiter); // Tokenize the string
    size_t index = 0;
    
    while (token != NULL) {
        // Skip leading spaces
        while (isspace((unsigned char)*token)) token++;

        // Trim trailing spaces
        char* end = token + strlen(token) - 1;
        while (end > token && isspace((unsigned char)*end)) end--;
        *(end + 1) = '\0'; // Null-terminate the trimmed token

        // Convert hex string to byte
        output[index++] = (uint8_t)strtol(token, NULL, 16);

        // Get the next token
        token = strtok(NULL, delimiter);
    }
}

static int parse_mac_address(const char *mac_str, uint8_t mac[6]) {
    int len = sscanf(mac_str, "%hhX-%hhX-%hhX-%hhX-%hhX-%hhX",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    return len;
}

bool check_config_cmd(behavior_config_t* config) {
    bool check_input = false;
    bool check_output = false;

    switch (config->input_cmd) {
        case INPUT_GPIO_CMD:
            check_input = true;
            break;
        case INPUT_ROTARY_CMD:
            check_input = true;
            break;
        case INPUT_I2C_CMD:
            check_input = true;
            break;
        case INPUT_NETWORK_CMD:
            check_input = true;
            break;
    }

    switch (config->output_cmd) {
        case OUTPUT_GPIO_CMD:
            check_output = true;
            break;
        case OUTPUT_WS2812_CMD:
            check_output = true;
            break;
        case OUTPUT_DISPLAY_CMD:
            check_output = true;
            break;
        case OUTPUT_NETWORK_CMD:
            check_output = true;
            break;
    }

    return check_input && check_output;
}

void littlefs_readLine_handler(char* file_name, char* str_buff, size_t len) {
    // len is the length of the string, not length of the byte representations in that string
    uint8_t remote_mac[6];
    parse_mac_address(file_name, remote_mac);
    printf("Parsed MAC: %02X:%02X:%02X:%02X:%02X:%02X\n\n", MAC2STR(remote_mac));

    uint8_t data_buff[len];
    parse_hex_string(str_buff, data_buff);

    // compare remote_mac to device_mac
    // int compare_mac = memcmp(remote_mac, device_mac, sizeof(device_mac)) == 0;

    behavior_config_t config;
    // copy remote_mac into config
    memcpy(config.remote_mac, remote_mac, sizeof(config.remote_mac));
    // copy data_buff starting from index 1 to config starting at index 6
    // forget the length bc it's off a  little bit
    memcpy((uint8_t*)&config+6, data_buff+1, sizeof(behavior_config_t));
    
    // behavior_config_t* config = (behavior_config_t*)(data_buff+1);

    int validate_code = data_buff[INDEX_VALIDATION] == BEHAVIOR_VALIDATION_CODE;
    
    if (validate_code && check_config_cmd(&config)) {
        ESP_LOGI(TAG, "Behavior Config:");
        print_bytes(data_buff, sizeof(behavior_config_t));

        // copy config into device_behaviors[behaviors_index]
        memcpy(&device_behaviors[behaviors_index++], &config, sizeof(behavior_config_t));
        printf("INPUT CMD: %02X\n", device_behaviors[behaviors_index-1].input_cmd);
        printf("OUTPUT CMD: %02X\n", device_behaviors[behaviors_index-1].output_cmd);
    }
}

void behavior_setup(uint8_t* esp_mac, behavior_output_cb cb) {
    output_callback = cb;
    memcpy(device_mac, esp_mac, sizeof(device_mac));

    behaviors_index = 0;
    littlefs_setup();
    littlefs_loadFiles("/littlefs/devices", littlefs_readLine_handler);
}

void behavior_process_gpio(input_gpio_t input_type, int8_t pin, uint16_t input_value) {
    for (int i=0; i<=behaviors_index; i++) {
        behavior_config_t config = device_behaviors[i];
        int compare_mac = memcmp(config.remote_mac, device_mac, sizeof(device_mac)) == 0;
        if (!compare_mac || config.input_cmd != INPUT_GPIO_CMD) continue;

        printf("IM HERE 333\n");
        if (config.input_data[0] == input_type && 
            config.input_data[1] == pin
        ) {
            output_callback(&config);
        }
    }
}

void behavior_process_rotary(uint16_t value, bool direction) {
    for (int i=0; i<=behaviors_index; i++) {
        behavior_config_t config = device_behaviors[i];
        int compare_mac = memcmp(config.remote_mac, device_mac, sizeof(device_mac)) == 0;
        if (!compare_mac || config.input_cmd != INPUT_ROTARY_CMD) continue;

        printf("IM HERE 444\n");
        output_callback(&config);
    }
}
