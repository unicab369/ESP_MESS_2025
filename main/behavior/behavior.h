#ifndef BEHAVIORS_H
#define BEHAVIORS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

//! INPUT COMMANDS
typedef enum __attribute__((packed)) {
    INPUT_CMD_GPIO = 0x01,
    INPUT_CMD_ROTARY = 0x02,
    INPUT_CMD_I2C = 0x03,
    INPUT_CMD_HTTP = 0x04,
    INPUT_CMD_ESPNOW = 0x05,
    INPUT_CMD_MQTT = 0x06
} input_command_t;


//! OUTPUT COMMANDS
typedef enum __attribute__((packed)) {
    OUTPUT_CMD_GPIO = 0xA0,
    OUTPUT_CMD_WS2812 = 0xB0,
    OUTPUT_CMD_DISPLAY = 0xC0
} output_command_t;

typedef enum __attribute__((packed)) {
    OUTPUT_GPIO_STATE = 0xA1,
    OUTPUT_GPIO_TOGGLE = 0xA2,
    OUTPUT_GPIO_FLICKER = 0xA3,
    OUTPUT_GPIO_FADE = 0xA4,
} output_gpio_t;

typedef enum __attribute__((packed)) {
    OUTPUT_WS2812_SINGLE,
    OUTPUT_WS2812_PATTERN
} output_ws2812_t;

//! BEHAVIORS
typedef struct {
    uint16_t behavior_code;
    input_command_t input;          // input command
    output_command_t output;        // output command
    uint8_t input_data[32];         // input data
    uint8_t output_data[32];        // output data
} behavior_config_t;


// Function prototypes
void behavior_setup(uint8_t* esp_mac);


#endif