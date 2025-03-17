#include "main.h"

#include "util_shared.h"

#define WIFI_ENABLED true

#if WIFI_ENABLED
    #include "app_network/app_network.h"
    #include "http/http.h"
#endif


static uint8_t esp_mac[6];

// Button event callback
void button_event_handler(button_event_t event, uint8_t pin, uint64_t pressed_time) {
    //! NOTE: button_event_t with input_gpio_t values need to match
    // behavior_process_gpio(event, pin, pressed_time);

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

            // espnow_controller_send();
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

#define MAX_ADC_SAMPLE 100
uint16_t adc_read_arr[MAX_ADC_SAMPLE];

static void http_request_handler(uint16_t **data, size_t *size) {
    *data = adc_read_arr;
    *size = MAX_ADC_SAMPLE;
}

void app_main(void) {
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
    // ssd1306_print_str("Hello World 333333!", 3, 5*5);
    // do_i2cdetect_cmd(SCL_PIN, SDA_PIN);

    #if WIFI_ENABLED
        app_network_setup();

        http_setup(&(http_interface_t){
            .on_file_fopen_cb   = mod_sd_fopen,
            .on_file_fread_cb   = mod_sd_fread,
            .on_file_fclose_cb  = mod_sd_fclose,
            .on_display_print   = display_print_str,
            .on_request_data    = http_request_handler
        });
    #endif

    mod_sd_spi_config(&(storage_sd_config_t) {
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
    mod_sd_test();

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
    led_fade_restart(1023, 400);        // Brightness, fade_duration

    ws2812_setup(WS2812_PIN);                   

    rotary_setup(ROTARY_CLK, ROTARY_DT, rotary_event_handler);
    button_click_setup(BUTTON_PIN, button_event_handler);
    // uart_setup(uart_read_handler);

    behavior_output_interface output_interface;
    // output_interface.on_gpio_set = 

    // behavior_setup(esp_mac, output_interface);

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

    // mod_mbedtls_setup();


    
    data_output_t data_output = {
        .type = DATA_OUTPUT_ANALOG,
        .len = 100
    };

    uint16_t adc_read_index = 0;

    memset(adc_read_arr, 0, sizeof(adc_read_arr));

    adc_single_read_t single_adc = {
        .gpio = 34,
        .unit = ADC_UNIT_INTF_1,
        .log_enabled = false
    };
    mod_adc_1read_setup(&single_adc);    

    // adc_continous_read_t continous_read = (adc_continous_read_t){
    //     .unit = ADC_UNIT_1
    // };

    // mod_adc_continous_setup(&continous_read);
    

    uint64_t interval_ref = 0;

    while (1) {
        uint64_t current_time = esp_timer_get_time();

        if (current_time - interval_ref > 1000000) {
            interval_ref = current_time;
            // mod_adc_1read(current_time, &single_adc);
            // mod_adc_continous_read(&continous_read);
        }

        if (current_time - interval_ref > pdMS_TO_TICKS(1)) {
            interval_ref = current_time;
            mod_adc_1read(current_time, &single_adc);
            adc_read_arr[adc_read_index++] = single_adc.raw;

            if (adc_read_index >= MAX_ADC_SAMPLE) {
                adc_read_index = 0;
                data_output.data = (uint16_t*)adc_read_arr;
                // app_network_push_data(data_output);
            }
        }
        
        #if CONFIG_IDF_TARGET_ESP32C3
            cdc_read_task();
        #endif
            
        // gpio_digital_loop(current_time);
        led_fade_loop(current_time);

        button_click_loop(current_time);
        rotary_loop(current_time);

        ws2812_loop(current_time);
        app_console_task();

        #if WIFI_ENABLED
            app_network_task(current_time);
        #endif

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
