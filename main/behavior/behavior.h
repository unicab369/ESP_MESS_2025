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

//! OUTPUT COMMANDS
typedef enum __attribute__((packed)) {
    OUTPUT_GPIO_SET = 0xA1,
    OUTPUT_GPIO_TOGGLE = 0xA2,
    OUTPUT_GPIO_PULSE = 0xA3,
    OUTPUT_GPIO_FADE = 0xA4,
    OUTPUT_WS2812_PULSE = 0xA5,
    OUTPUT_WS2812_PATTERN = 0xA6
} output_gpio_t;

//! BEHAVIORS
typedef struct {
    uint8_t remote_mac[6];
    input_command_t input_cmd;
    uint8_t input_data[16];
    uint8_t output_cmd;
    uint8_t output_data[16];
} behavior_config_t;


typedef void (*gpio_set_cb_t)(uint8_t pin, uint16_t val);
typedef void (*gpio_toogle_cb_t)(uint8_t pin);
typedef void (*gpio_pulse_cb_t)(uint8_t pin, uint8_t count, uint32_t repeat_duration);
typedef void (*gpio_fade_cb_t)(uint8_t pin, uint32_t output_threshold, uint32_t duration_ms);
typedef void (*ws2812_pulse_cb_t)();


typedef struct {
    gpio_set_cb_t on_gpio_set;
    gpio_toogle_cb_t on_gpio_toggle;
    gpio_pulse_cb_t on_gpio_pulse;
    gpio_fade_cb_t on_gpio_fade;
} behavior_output_interface;


// Function prototypes
void behavior_setup(uint8_t* esp_mac, behavior_output_interface interface);
void behavior_process_gpio(input_gpio_t input_type, int8_t pin, uint16_t input_value);
void behavior_process_rotary(uint16_t value, bool direction);

#endif