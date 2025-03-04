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
#include "ws2812.h"
#include "timer_pulse.h"


#define ESPNOW_ENABLED false

#if ESPNOW_ENABLED
    #include "espnow_driver.h"

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
#endif

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

// Button event callback
void button_event_handler(button_event_t event, uint8_t pin, uint64_t pressed_time) {
    //! NOTE: button_event_t with input_gpio_t values need to match
    behavior_process_gpio(event, pin, pressed_time);

    switch (event) {
        case BUTTON_SINGLE_CLICK:
            gpio_toggle_obj obj_a = {
                .object_index = 0,
                .timer_config = {
                    .pulse_count = 1,
                    .pulse_time_ms = 100,
                    .wait_time_ms = 1000,
                }
            };

            led_fade_stop();
            gpio_digital_config(obj_a);
            // led_toggle_switch();
            break;
        case BUTTON_DOUBLE_CLICK:
            gpio_digital_stop(0);
            led_fade_restart(1023, 500);        // Brightness, fade_duration
            break;
        case BUTTON_LONG_PRESS:
            gpio_toggle_obj obj_b = {
                .object_index = 0,
                .timer_config = {
                    .pulse_count = 3,
                    .pulse_time_ms = 100,
                    .wait_time_ms = 1000,
                }
            };

            led_fade_stop();        
            gpio_digital_config(obj_b);
            break;
    }
}

void rotary_event_handler(int16_t value, bool direction) {
    behavior_process_rotary(value, direction);
}

void uart_read_handler(uint8_t* data, size_t len) {
    ESP_LOGI(TAG, "IM HERE");
}



void app_main(void)
{
    ESP_LOGI(TAG, "APP START");
    #if CONFIG_IDF_TARGET_ESP32C3
        cdc_setup();
    #endif

    gpio_digital_setup(LED_FADE_PIN);

    gpio_toggle_obj obj0 = {
        .object_index = 0,
        .timer_config = {
            .pulse_count = 2,
            .pulse_time_ms = 100,
            .wait_time_ms = 1000,
        }
    };

    gpio_digital_config(obj0);

    led_fade_setup(LED_FADE_PIN);
    led_fade_restart(1023, 500);        // Brightness, fade_duration

    ws2812_setup();

    rotary_setup(ROTARY_CLK, ROTARY_DT, rotary_event_handler);
    button_click_setup(BUTTON_PIN, button_event_handler);
    uart_setup(uart_read_handler);

    #if ESPNOW_ENABLED
        espnow_setup(esp_mac, espnow_message_handler);
        ESP_LOGW(TAG, "ESP mac: %02x:%02x:%02x:%02x:%02x:%02x", MAC2STR(esp_mac));
    #endif
    
    behavior_output_interface output_interface;
    // output_interface.on_gpio_set = 

    behavior_setup(esp_mac, output_interface);

    ws2812_cyclePulse_t ojb1 = {
        .obj_index = 0,
        .led_index = 4,
        .rgb = { .red = 150, .green = 0, .blue = 0 },
        .config = {
            .pulse_count = 1,
            .pulse_time_ms = 100,
            .wait_time_ms = 1000,
        }
    };
    ws2812_load_pulse(ojb1);


    ws2812_cyclePulse_t ojb2 = {
        .obj_index = 1,
        .led_index = 3,
        .rgb =  { .red = 0, .green = 150, .blue = 0 },
        .config = {
            .pulse_count = 2,
            .pulse_time_ms = 100,
            .wait_time_ms = 1000,
        }
    };
    ws2812_load_pulse(ojb2);


    ws2812_cycleFade_t obj3 = {
        .led_index = 1,
        .active_channels = { .red = 0xFF, .green = 0, .blue = 0xFF },
        .config = {
            .current_value = 0,
            .is_switched = true,
            .increment = 5,
            .max_value = 150,
            .refresh_time_uS = 40000,
        }
    };

    ws2812_load_fadeColor(obj3, 0);

    ws2812_cycleFade_t obj4 = {
        .led_index = 2,
        .active_channels = { .red = 0xFF, .green = 0, .blue = 0 },
        .config = {
            .current_value = 0,
            .is_switched = true,
            .increment = 5,
            .max_value = 150,       // hue max 360
            .refresh_time_uS = 20000,
        }
    };
    ws2812_load_fadeColor(obj4, 1);


    while (1) {
        uint64_t current_time = esp_timer_get_time();
        
        #if CONFIG_IDF_TARGET_ESP32C3
            cdc_read_task();
        #endif

        gpio_digital_loop(current_time);
        led_fade_loop(current_time);

        // uart_run();
        button_click_loop(current_time);
        rotary_loop(current_time);

        // espnow_controller_send();
        ws2812_loop(current_time);

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}