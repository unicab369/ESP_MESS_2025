#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#include "esp_timer.h"
#include "esp_err.h"

#include "led_toggle.h"
#include "led_fade.h"
#include "button_click.h"
#include "rotary_driver.h"
#include "uart_driver.h"
#include "behavior/behavior.h"
#include "espnow_driver.h"
#include "ws2812.h"
#include "timer_pulse.h"

#if CONFIG_IDF_TARGET_ESP32C3
    #include "cdc_driver.h"
    #define LED_FADE_PIN 2
    #define BLINK_PIN 10
    #define BUTTON_PIN 9
#elif CONFIG_IDF_TARGET_ESP32
    #define LED_FADE_PIN 22
    #define BLINK_PIN 5
    // #define BUTTON_PIN 16
    #define BUTTON_PIN 23
    #define ROTARY_CLK 15
    #define ROTARY_DT 13
#endif

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

static const char *TAG = "MAIN";

uint8_t esp_mac[6];

timer_pulse_obj_t led_pulse_objs[1];

// Button event callback
void button_event_handler(button_event_t event, uint8_t pin, uint64_t pressed_time) {
    //! NOTE: button_event_t with input_gpio_t values need to match
    behavior_process_gpio(event, pin, pressed_time);

    switch (event) {
        case BUTTON_SINGLE_CLICK:
            timer_pulse_config_t config_1 = {
                .pulse_count = 2,
                .pulse_time_ms = 100,
                .wait_time_ms = 1000,
                .callback = led_toggle
            };

            led_fade_stop();
            // led_toggle_pulses(1, 0);
            led_toggle_pulses(config_1, &led_pulse_objs[0]);
            // led_toggle_switch();
            break;
        case BUTTON_DOUBLE_CLICK:
            led_toggle_stop(&led_pulse_objs[0]);
            led_fade_restart(1023, 500);        // Brightness, fade_duration
            break;
        case BUTTON_LONG_PRESS:
            timer_pulse_config_t config_2 = {
                .pulse_count = 3,
                .pulse_time_ms = 100,
                .wait_time_ms = 1000,
                .callback = led_toggle
            };

            led_fade_stop();
            led_toggle_pulses(config_2, &led_pulse_objs[0]);
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

#define PULSE_OJB_COUNT 2

// void ws2812_toggle(bool state) {
//     printf("IM HERE \n");
// }

void app_main(void)
{
    ESP_LOGI(TAG, "APP START");
    #if CONFIG_IDF_TARGET_ESP32C3
        cdc_setup();
    #endif

    led_toggle_setup(LED_FADE_PIN); 
    led_fade_setup(LED_FADE_PIN);
    led_fade_restart(1023, 500);        // Brightness, fade_duration

    ws2812_setup();

    rotary_setup(ROTARY_CLK, ROTARY_DT, rotary_event_handler);
    button_click_setup(BUTTON_PIN, button_event_handler);
    uart_setup(uart_read_handler);

    espnow_setup(esp_mac, espnow_message_handler);
    ESP_LOGW(TAG, "ESP mac: %02x:%02x:%02x:%02x:%02x:%02x", MAC2STR(esp_mac));

    
    behavior_output_interface output_interface;
    // output_interface.on_gpio_set = 

    behavior_setup(esp_mac, output_interface);

    timer_pulse_obj_t ws2812_pulse_objs[PULSE_OJB_COUNT];

    timer_pulse_config_t config1 = {
        .pulse_count = 1,
        .pulse_time_ms = 500,
        .wait_time_ms = 500,
        .callback = ws2812_toggle
    };
    ws2812_load_obj(config1, &ws2812_pulse_objs[0]);
    
    timer_pulse_config_t config2 = {
        .pulse_count = 1,
        .pulse_time_ms = 200,
        .wait_time_ms = 200,
        .callback = ws2812_toggle2
    };
    ws2812_load_obj(config2, &ws2812_pulse_objs[1]);


    while (1) {
        uint64_t current_time = esp_timer_get_time();
        
        #if CONFIG_IDF_TARGET_ESP32C3
            cdc_read_task();
        #endif

        // led_toggle_loop(current_time, led_pulse_objs, sizeof(led_pulse_objs));
        led_fade_loop(current_time);

        // uart_run();
        button_click_loop(current_time);
        rotary_loop(current_time);

        // espnow_controller_send();
        ws2812_loop(current_time, ws2812_pulse_objs, PULSE_OJB_COUNT);

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}