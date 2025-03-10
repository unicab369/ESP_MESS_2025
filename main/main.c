#include "main.h"


#if WIFI_ENABLED
    #include "wifi.h"
    #include "wifi_nan.h"
    #include "espnow_driver.h"
    #include "udp_socket/udp_socket.h"
    #include "ntp/ntp.h"
    #include "tcp_socket/tcp_socket.h"
    #include "http/http.h"

    // #include "modbus/modbus.h"

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

static void print_hex(const char *label, const unsigned char *buf, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", buf[i]);
    }
    printf("\n");
}

void app_main(void)
{   
    //! nvs_flash required for WiFi, ESP-NOW, and other stuff.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "APP START");

    #if CONFIG_IDF_TARGET_ESP32C3
        cdc_setup();
    #endif


    display_setup(SCL_PIN, SDA_PIN, SSD_1306_ADDR);

    // ssd1306_setup(SCL_PIN, SDA_PIN, SSD_1306_ADDR);

    // ssd1306_print_str("Hello World aaaabbbbccccddddeeeffff!", 0);
    // ssd1306_print_str("Hello World 222222!", 1);
    // ssd1306_print_str("Hello World 333333!", 2);
    // ssd1306_print_str("Hello World 333333!", 3, 5*5);
    // do_i2cdetect_cmd(SCL_PIN, SDA_PIN);

    #if WIFI_ENABLED
        wifi_setup(&(app_wifi_interface_t) {
            .on_display_print = display_print_str,
            .max_retries = 10,
        });

        wifi_configure_softAp(AP_WIFI_SSID, AP_WIFI_PASSWORD, 1);
        wifi_configure_sta(WIFI_SSID, WIFI_PASSWORD);

    
        http_setup(&(http_interface_t){
            .on_file_fopen_cb = storage_sd_fopen,
            .on_file_fread_cb = storage_sd_fread,
            .on_file_fclose_cb = storage_sd_fclose,
            .on_display_print = display_print_str
        });
        wifi_connect();

        espnow_setup(esp_mac, espnow_message_handler);

        char mac_str[32];
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(esp_mac));
        ESP_LOGW(TAG, "ESP mac: %s\n", mac_str);
        // display_print_str(mac_str, 0);

        // wifi_scan();

        // wifi_nan_subscribe();
        // wifi_nan_publish();
    #endif

    storage_sd_spi_config(&(storage_sd_config_t) {
        .mmc_enabled = true,

        //! NOTE: for MMC D3 or CS needs to be pullup if not used otherwise it will go into SPI mode
        .mmc = {
            .enable_width4 = false,
            .clk = MMC_CLK,
            .di = MMC_DI,
            .d0 = MMC_D0
        },
        .spi = {
            .mosi = SPI_MOSI,
            .miso = SPI_MISO,
            .sclk = SPI_CLK,
            .cs = SPI_CS
        },
    });
    // storage_sd_test();

    app_console_setup();

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

    ws2812_setup(WS2812_PIN);                   

    rotary_setup(ROTARY_CLK, ROTARY_DT, rotary_event_handler);
    button_click_setup(BUTTON_PIN, button_event_handler);
    // uart_setup(uart_read_handler);

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
            .direction = true,
            .is_bounced = true,
            .increment = 5,
            .start_index = 0,
            .end_index = 150,       // hue max 360
            .refresh_time_uS = 20000,
            .last_refresh_time = 0
        }
    };
    ws2812_load_fadeColor(obj3, 0);

    ws2812_cycleFade_t obj4 = {
        .led_index = 2,
        .active_channels = { .red = 0xFF, .green = 0, .blue = 0 },
        .config = {
            .current_value = 0,
            .direction = true,
            .is_bounced = false,
            .increment = 5,
            .start_index = 0,
            .end_index = 150,       // hue max 360
            .refresh_time_uS = 50000,
            .last_refresh_time = 0
        }
    };
    ws2812_load_fadeColor(obj4, 1);

    // app_mbedtls_setup();


    uint64_t second_interval_check = 0;

    while (1) {
        uint64_t current_time = esp_timer_get_time();
        
        #if CONFIG_IDF_TARGET_ESP32C3
            cdc_read_task();
        #endif
            
        // gpio_digital_loop(current    _time);
        led_fade_loop(current_time);

        button_click_loop(current_time);
        // rotary_loop(current_time);

        ws2812_loop(current_time);
        app_console_run();

        #if WIFI_ENABLED
            if (current_time - second_interval_check > 1000000) {
                second_interval_check = current_time;

            }

            // wifi_nan_checkPeers(current_time);
            // wifi_nan_sendData(current_time);

            wifi_status_t status = wifi_check_status(current_time);
            
            if (status == WIFI_STATUS_CONNECTED) {
                ntp_status_t ntp_status = ntp_task(current_time);

                //! tcp sockets block, need to find a solution
                // tcp_status_t tcp_status = tcp_server_socket_setup(current_time);
                // if (tcp_status == TCP_STATUS_SETUP) {
                //     tcp_server_socket_task(current_time);
                // }
                // tcp_client_socket_task(current_time);

                //! udp sockets block, need to find a solution
                // udp_status_t udp_status = udp_server_socket_setup(current_time);
                // if (udp_status == UDP_STATUS_SETUP) {
                //     udp_server_socket_task();
                //     // udp_client_socket_send(current_time);
                // }
            }

            // espnow_controller_send();
        #endif

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}