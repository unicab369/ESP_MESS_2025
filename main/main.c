#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#include "esp_err.h"

#include "led_toggle.h"
#include "led_fade.h"
#include "button_click.h"
#include "rotary_driver.h"
#include "uart_driver.h"
// #include "espnow_controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
// #include "littlefs_driver.h"
// #include "my_littlefs.h"

#if CONFIG_IDF_TARGET_ESP32C3
    #include "cdc_driver.h"
    #define LED_FADE_PIN 2
    #define BLINK_PIN 10
    #define BUTTON_PIN 9
#elif CONFIG_IDF_TARGET_ESP32
    #define LED_FADE_PIN 22
    #define BLINK_PIN 5
    #define BUTTON_PIN 16
    #define ROTARY_CLK 15
    #define ROTARY_DT 13
#endif

static const char *TAG = "MAIN";


typedef enum __attribute__((packed)) {
    CMD_GPIO,
    CMD_WS2812,
    CMD_DISPLAY
} command_t;

typedef enum __attribute__((packed)) {
    GPIO_STATE,
    GPIO_TOGGLE,
    GPIO_FLICKER,
    GPIO_FADE,
} gpio_output_t;

typedef enum __attribute__((packed)) {
    WS2812_SINGLE,
    WS2812_PATERN
} ws2812_output_t;


void gpio_set_state(uint8_t pin, uint8_t value) {
    uint8_t data[32];

    data[0] = CMD_GPIO;
    data[1] = GPIO_STATE;
    data[2] = pin;
    data[3] = value;
}

// typedef struct {
//     gpio_action_t action_type;
// } gpio_command_action_t;


// Button event callback
void button_event_handler(button_event_t event, uint64_t duration) {
    switch (event) {
        case BUTTON_SINGLE_CLICK:
            ESP_LOGI(TAG, "Single click detected!\n");
            led_toggle_switch();
            break;
        case BUTTON_DOUBLE_CLICK:
            ESP_LOGI(TAG, "Double click detected!\n");
            led_toggle_pulses(1, 0);
            break;
        case BUTTON_LONG_PRESS:
            ESP_LOGI(TAG, "duration = %lld", duration/1000);
            led_toggle_pulses(3, 1000);
            break;
    }
}

void rotary_event_handler(int16_t value, bool direction) {
    const char* directionStr = direction ? "CW" : "CCW";
    ESP_LOGI(TAG, "rotary = %d, direction: %s", value, directionStr);
}

void uart_read_handler(uint8_t* data, size_t len) {
    ESP_LOGI(TAG, "IM HERE");
}


void app_main(void)
{
    #if CONFIG_IDF_TARGET_ESP32C3
        cdc_setup();
    #endif
    ESP_LOGI(TAG, "APP START");

    led_toggle_setup(BLINK_PIN);
    // led_toggle_pulses(3, 1000);

    led_fade_setup(LED_FADE_PIN, 5000);             // 5 kHz frequency
    led_fade_start(1023, 500, 10);                  // Brightness, fade_duration, update_frequency

    rotary_setup(ROTARY_CLK, ROTARY_DT, rotary_event_handler);
    button_click_setup(BUTTON_PIN, button_event_handler);
    uart_setup(uart_read_handler);
    // espnow_controller_setup();

    // littlefs_setup();
    // littlefs_test();

    while (1) {
        #if CONFIG_IDF_TARGET_ESP32C3
            cdc_read_task();
        #endif

        led_toggle_loop();
        led_fade_loop();

        // uart_run();
        button_click_loop();
        rotary_loop();

        // espnow_controller_send();

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}