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
#include "behavior/behavior.h"
#include "espnow_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"

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

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

static const char *TAG = "MAIN";

uint8_t esp_mac[6];

// Button event callback
void button_event_handler(button_event_t event, uint8_t pin, uint64_t pressed_time) {
    //! NOTE: button_event_t with input_gpio_t values need to match
    behavior_process_gpio(event, pin, pressed_time);

    switch (event) {
        case BUTTON_SINGLE_CLICK:
            led_fade_stop();
            // led_toggle_pulses(1, 0);
            // led_toggle_pulses(3, 1000);
            led_toggle_switch();
            break;
        case BUTTON_DOUBLE_CLICK:
            led_toggle_stop();
            led_fade_restart(1023, 500);        // Brightness, fade_duration
            break;
        case BUTTON_LONG_PRESS:
            led_fade_stop();
            led_toggle_pulses(3, 1000);
            break;
    }
}

void rotary_event_handler(int16_t value, bool direction) {
    behavior_process_rotary(value, direction);
}

void uart_read_handler(uint8_t* data, size_t len) {
    ESP_LOGI(TAG, "IM HERE");
}

void espnow_message_handler(espnow_received_message_t received_message) {
    ESP_LOGW(TAG,"received data:");
    ESP_LOGI(TAG, "Source Address: %02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(received_message.src_addr));

    ESP_LOGI(TAG, "rssi: %d", received_message.rssi);
    ESP_LOGI(TAG, "channel: %d", received_message.channel);
    ESP_LOGI(TAG, "group_id: %d", received_message.message->group_id);
    ESP_LOGI(TAG, "msg_id: %d", received_message.message->msg_id);
    ESP_LOGI(TAG, "access_code: %u", received_message.message->access_code);
    ESP_LOGI(TAG, "Data: %.*s", sizeof(received_message.message->data), received_message.message->data);
}


void behavior_handle_output(behavior_config_t *config) {
    switch (config->output_cmd) {
        case OUTPUT_GPIO_CMD:
            output_gpio_t output_type = config->output_data[0];
            
            switch (output_type) {
                case OUTPUT_GPIO_STATE:
                    break;
                case OUTPUT_GPIO_TOGGLE:
                    break;
                case OUTPUT_GPIO_FADE:
                    break;    
            }
            break;
        case OUTPUT_WS2812_CMD:
            break;
        case OUTPUT_DISPLAY_CMD:
            break;
        case OUTPUT_NETWORK_CMD:
            break;
    }
}


void espnow_controller_send() {
    uint8_t dest_mac[6] = { 0xAA };
    uint8_t data[32] = { 0xBB };

    espnow_message_t message = {
        .access_code = 33,
        .group_id = 11,
        .msg_id = 12,
        .time_to_live = 15,
    };

    memcpy(message.target_addr, dest_mac, sizeof(message.target_addr));
    memcpy(message.data, data, sizeof(message.data));

    espnow_send((uint8_t*)&message, sizeof(message));
}

void app_main(void)
{
    #if CONFIG_IDF_TARGET_ESP32C3
        cdc_setup();
    #endif
    ESP_LOGI(TAG, "APP START");

    led_toggle_setup(LED_FADE_PIN); 
    led_fade_setup(LED_FADE_PIN);
    led_fade_restart(1023, 500);        // Brightness, fade_duration

    rotary_setup(ROTARY_CLK, ROTARY_DT, rotary_event_handler);
    button_click_setup(BUTTON_PIN, button_event_handler);
    uart_setup(uart_read_handler);

    espnow_setup(esp_mac, espnow_message_handler);
    ESP_LOGW(TAG, "ESP mac: %02x:%02x:%02x:%02x:%02x:%02x", MAC2STR(esp_mac));
    behavior_setup(esp_mac, behavior_handle_output);

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