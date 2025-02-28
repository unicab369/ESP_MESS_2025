#ifndef BEHAVIORS_H
#define BEHAVIORS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

//! OUTPUT COMMANDS
typedef enum __attribute__((packed)) {
    OUTPUT_CMD_GPIO,
    OUTPUT_CMD_WS2812,
    OUTPUT_CMD_DISPLAY
} output_command_t;

typedef enum __attribute__((packed)) {
    OUTPUT_GPIO_STATE,
    OUTPUT_GPIO_TOGGLE,
    OUTPUT_GPIO_FLICKER,
    OUTPUT_GPIO_FADE,
} output_gpio_t;

typedef enum __attribute__((packed)) {
    OUTPUT_WS2812_SINGLE,
    OUTPUT_WS2812_PATTERN
} output_ws2812_t;


//! INPUT COMMANDS
typedef enum __attribute__((packed)) {
    INPUT_CMD_GPIO,
    INPUT_CMD_ROTARY,
    INPUT_CMD_I2C,
    INPUT_CMD_HTTP,
    INPUT_CMD_ESPNOW,
    INPUT_CMD_MQTT
} input_command_t;


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