#ifndef BEHAVIORS_H
#define BEHAVIORS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


//! INPUT COMMANDS
typedef enum __attribute__((packed)) {
    INPUT_GPIO_CMD = 0x10,
    INPUT_ROTARY_CMD = 0x20,
    INPUT_I2C_CMD = 0x30,
    INPUT_NETWORK_CMD = 0x40,
} input_command_t;

typedef enum __attribute__((packed)) {
    INPUT_GPIO_1CLICK = 0x11,
    INPUT_GPIO_2CLICK = 0x12,
    INPUT_GPIO_PRESS = 0x13,
    INPUT_GPIO_PIR = 0x14,
    INPUT_GPIO_ANALOG = 0x15
} input_gpio_t;

typedef struct __attribute__((packed)) {
    uint8_t input_id;
    uint8_t data[32];
} input_config_t;

//! OUTPUT COMMANDS
typedef enum __attribute__((packed)) {
    OUTPUT_GPIO_CMD = 0xA0,
    OUTPUT_WS2812_CMD = 0xB0,
    OUTPUT_DISPLAY_CMD = 0xC0,
    OUTPUT_NETWORK_CMD = 0xD0
} output_command_t;

typedef enum __attribute__((packed)) {
    OUTPUT_GPIO_STATE = 0xA1,
    OUTPUT_GPIO_TOGGLE = 0xA2,
    OUTPUT_GPIO_FADE = 0xA4,
} output_gpio_t;

typedef enum __attribute__((packed)) {
    OUTPUT_WS2812_SINGLE,
    OUTPUT_WS2812_PATTERN
} output_ws2812_t;

//! BEHAVIORS
typedef struct {
    uint8_t remote_mac[6];
    input_command_t input_cmd;
    uint8_t input_data[16];
    output_command_t output_cmd;
    uint8_t output_data[16];
} behavior_config_t;

typedef void (*behavior_output_cb)(behavior_config_t* config);

// Function prototypes
void behavior_setup(uint8_t* esp_mac, behavior_output_cb callback);
void behavior_process_gpio(input_gpio_t input_type, int8_t pin, uint16_t input_value);
void behavior_process_rotary(uint16_t value, bool direction);

#endif