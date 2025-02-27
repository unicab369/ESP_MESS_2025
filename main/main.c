#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_err.h"

#include "led_toggle.h"
#include "led_fade.h"
#include "button_click.h"
#include "rotary_driver.h"
#include "uart_driver.h"
// #include "espnow_driver.h"
#include "espnow_controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
// #include "littlefs_driver.h"
#include "my_littlefs.h"

#if CONFIG_IDF_TARGET_ESP32C3
    #include "cdc_driver.h"
    #define LED_FADE_PIN 2
    #define BLINK_PIN 10
    #define BUTTON_PIN 9
#elif CONFIG_IDF_TARGET_ESP32
    #define LED_FADE_PIN 22
    #define BLINK_PIN 2
    #define BUTTON_PIN 16
    #define ROTARY_CLK 15
    #define ROTARY_DT 13
#endif

static const char *TAG = "MAIN";

// Button event callback
void button_event_handler(button_event_t event, uint64_t duration) {
    switch (event) {
        case BUTTON_EVENT_SINGLE_CLICK:
            ESP_LOGI(TAG, "Single click detected!\n");
            break;
        case BUTTON_EVENT_DOUBLE_CLICK:
            ESP_LOGI(TAG, "Double click detected!\n");
            break;
        case BUTTON_EVENT_LONG_PRESS:
            ESP_LOGI(TAG, "duration = %lld", duration/1000);
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

    led_config_t led_config = {
        .blink_count = 3,
        .toggle_time_ms = 100,
        .wait_time_ms = 1000,
        .invert_state = true
    };

    led_toggle_setup(BLINK_PIN, led_config);
    led_fade_setup(LED_FADE_PIN, 5000);              // 5 kHz frequency
    led_fade_start(1023, 500, 10);              // Brightness, fade_duration, update_frequency

    rotary_setup(ROTARY_CLK, ROTARY_DT, rotary_event_handler);
    button_click_setup(BUTTON_PIN, button_event_handler);
    uart_setup(uart_read_handler);
    // espnow_setup();
    espnow_controller_setup();

    littlefs_setup();
    littlefs_test();

    while (1) {
        #if CONFIG_IDF_TARGET_ESP32C3
            cdc_read_task();
        #endif

        led_toggle_run();
        led_fade_run();

        // uart_run();
        button_click_run();
        rotary_run();

        // espnow_send();
        // espnow_controller_send();

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
