#include <stdio.h>
#include <string.h>
#include "led_toggle.h"
#include "led_fade.h"
#include "button_click.h"
#include "uart_controller.h"
#include "cdc_driver.h"
#include "espNow_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"

// Button event callback
void button_event_handler(button_event_t event, uint64_t duration) {
    switch (event) {
        case BUTTON_EVENT_SINGLE_CLICK:
            ESP_LOGI("TAG", "Single click detected!\n");
            break;
        case BUTTON_EVENT_DOUBLE_CLICK:
            ESP_LOGI("TAG", "Double click detected!\n");
            break;
        case BUTTON_EVENT_LONG_PRESS:
            ESP_LOGI("TAG", "duration = %lld", duration/1000);
            break;
    }
}

void aurt_read_handler(uint8_t* data, size_t len) {
    ESP_LOGI("TAG", "IM HERE");
}

void app_main(void)
{
    // led_config_t led_config = {
    //     .blink_count = 3,         // Blink 5 times
    //     .toggle_time_ms = 100,    // Toggle every 500ms
    //     .wait_time_ms = 1000,       // Wait 200ms between blinks
    //     .invert_state = true
    // };

    // // Initialize LED toggling
    // led_toggle_init(2, led_config);     // GPIO_2

    // Initialize LED fading
    led_fade_setup(2, 5000);             // GPIO_2, 5 kHz frequency
    led_fade_start(1023, 500, 10);      // Brightness, fade_duration, update_frequency

    button_click_setup(10, button_event_handler);
    uart_setup(21, 20, aurt_read_handler);
    cdc_setup();

    espNow_setup();

    while (1) {
        // led_toggle_update();
        led_fade_update();

        uart_run();
        button_click_run();
        cdc_read_task();

        // espnow_send();

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}