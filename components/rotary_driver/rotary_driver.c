#include "rotary_driver.h"

#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define ROTARY_UPDATE_TIME 50000        // 50ms
#define DEBOUNCE_TIME 1000              // 1ms

static int16_t value = 0;
static int last_clk_state;
static int last_dt_state;
static bool direction;
static uint8_t CLK_PIN;
static uint8_t DT_PIN;
static uint64_t last_update_time = 0;
static uint64_t last_debounce_time = 0;

static rotary_callback_t event_callback;

void rotary_setup(uint8_t clk_pin, uint8_t dt_pin, rotary_callback_t callback) {
    CLK_PIN = clk_pin;
    DT_PIN = dt_pin;
    event_callback = callback;

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<CLK_PIN) | (1ULL<<DT_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
        
    last_clk_state = gpio_get_level(CLK_PIN);
    last_dt_state = gpio_get_level(DT_PIN);
}

void rotary_loop() {
    int clk_state = gpio_get_level(CLK_PIN);
    int dt_state = gpio_get_level(DT_PIN);
    uint64_t current_time = esp_timer_get_time();

    // Check for state change (with debouncing)
    if (clk_state != last_clk_state || dt_state != last_dt_state) {
        // Ignore changes that happen too quickly (debouncing)
        if (current_time - last_debounce_time > DEBOUNCE_TIME) {
            // Determine the direction of rotation
            if (clk_state != last_clk_state) {
                if (clk_state == dt_state) {
                    value--;
                    direction = false;  // Counter Clockwise
                } else {
                    value++;
                    direction = true;   // Clockwise
                }

                // Trigger the callback if enough time has passed since the last update
                if (current_time - last_update_time > ROTARY_UPDATE_TIME) {
                    event_callback(value, direction);
                    printf("rotary = %d, direction: %u\n", value, direction);
                    last_update_time = current_time;
                }
            }

            // Update the last state and debounce time
            last_clk_state = clk_state;
            last_dt_state = dt_state;
            last_debounce_time = current_time;
        }
    }
}