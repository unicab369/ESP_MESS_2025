#include "main.h"

#include "mod_utility.h"
#include "mod_adc.h"

#include "mod_mbedtls.h"
#include "mod_ws2812.h"
#include "mod_button.h"
#include "mod_rotary.h"

#include "mod_i2s.h"
#include "devices/app_serial.h"

#include "driver/spi_master.h"

#define WIFI_ENABLED 0

#if WIFI_ENABLED
    #include "app_network/app_network.h"
    #include "http/http.h"
#endif

static const char *MTAG = "MAIN";
static uint8_t esp_mac[6];

#define MAX_ADC_SAMPLE 100
uint16_t adc_read_arr[MAX_ADC_SAMPLE];

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

void rotary_event_handler(int16_t value, int8_t direction) {
    app_serial_setMode(direction);
    behavior_process_rotary(value, direction);
}

void uart_read_handler(uint8_t* data, size_t len) {
    ESP_LOGI(MTAG, "IM HERE");
}

static void print_hex(const char *label, const unsigned char *buf, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", buf[i]);
    }
    printf("\n");
}

static void http_request_handler(uint16_t **data, size_t *size) {
    *data = adc_read_arr;
    *size = MAX_ADC_SAMPLE;
}


#include "sx127x/mod_sx127x.c"

#include "mod_spi.h"


void app_main(void) {
    mod_sx127_listen();

    return;

    //! nvs_flash required for WiFi, ESP-NOW, and other stuff.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(MTAG, "APP START");

    #if CONFIG_IDF_TARGET_ESP32C3
        cdc_setup();
    #endif

    app_serial_i2c_setup(SCL_PIN, SDA_PIN, 0);
    app_serial_i2c_setup(SCL_PIN2, SDA_PIN2, 1);
    
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

    //# Init SPI1 peripherals
    M_Spi_Conf spi_config_a = {
        .host = 1,
        .mosi = SPI_MOSI,
        .miso = SPI_MISO,
        .clk = SPI_CLK,
        .cs = SPI_CS0,
    };

    ret = mod_spi_init(&spi_config_a, 20E6);
    if (ret == ESP_OK) {
        // spi_setup_sdCard(spi_config_a.host, spi_config_a.cs);

        // //! DC and RST pins are required for ST7735
        // spi_config_a.dc = SPI_DC;
        // spi_config_a.rst = SPI_RST;

        // //! Reset the device first
        // // mod_spi_setup_rst(SPI_RST);
        // spi_setup_st7735(&spi_config_a, SPI_CS_X0);

        // setup_loRa(&spi_config_a.spi_handle);
    }

    //# Init SPI2 peripherals
    M_Spi_Conf spi_config_b = {
        .host = 2,
        .mosi = SPI2_MOSI,
        .miso = SPI2_MISO,
        .clk = SPI2_CLK,
        .cs = SPI2_CS0,
    };

    ret = mod_spi_init(&spi_config_b, 20E6);
    if (ret == ESP_OK) {
        // spi_setup_sdCard(spi_config_b.host, spi_config_b.cs);

        //! DC and RST pins are required for ST7735
        spi_config_b.dc = SPI_DC;
        spi_config_b.rst = SPI_RST;

        //! Reset the device first
        // mod_spi_setup_rst(SPI_RST);
        // spi_setup_st7735(&spi_config_b, SPI_CS_X1);

        // mod_spi_setup_rst(SPI_RST);
        // setup_loRa(&spi_config_b.spi_handle);
    }


    //! Audio test
    // mod_audio_setup(SPI2_BUSY);


    // gpio_digital_setup(LED_FADE_PIN);

    gpio_toggle_obj obj0 = {
        .object_index = 0,
        .timer_config = {
            .pulse_count = 2,
            .pulse_time_ms = 100,
            .wait_time_ms = 1000,
        }
    };

    // gpio_digital_config(obj0);

    // rotary_setup(ROTARY_CLK, ROTARY_DT, rotary_event_handler);

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
    // ws2812_load_pulse(ojb1);

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
    // ws2812_load_pulse(ojb2);

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
    // ws2812_load_fadeColor(obj3, 0);

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
    // ws2812_load_fadeColor(obj4, 1);

    // mod_mbedtls_setup();
    
    // data_output_t data_output = {
    //     .type = DATA_OUTPUT_ANALOG,
    //     .len = 100
    // };

    uint16_t adc_read_index = 0;

    memset(adc_read_arr, 0, sizeof(adc_read_arr));

    mod_adc_1read_init(ADC_UNIT_INTF_1, false, false);

    // adc_single_read_t single_adc = {
    //     .gpio = GPIO_34,
    // };
    // mod_adc_1read_setup(&single_adc);    

    // adc_single_read_t mic_adc = {
    //     .gpio = MIC_IN,
    // };
    // mod_adc_1read_setup(&mic_adc);

    // adc_single_read_t pir_adc = {
    //     .gpio = PIR_IN,
    // };
    // mod_adc_1read_setup(&pir_adc);


    // adc_continous_read_t continous_read = (adc_continous_read_t){
    //     .unit = ADC_UNIT_1
    // };

    // mod_adc_continous_setup(&continous_read);
    
    // mod_i2s_setup();
    // mod_i2s_send();

    uint64_t interval_ref = 0;
    uint64_t interval_ref2 = 0;

    // #include "mod_nimBLE.h"
    #include "mod_BLEScan.h"

    // mod_BLEScan_setup();
    // mod_nimbleBLE_setup(false);
    // xTaskCreate(mod_heart_rate_task, "Heart Rate", 4*1024, NULL, 5, NULL);

    // button_click_setup(BUTTON_PIN, button_event_handler);

    led_fade_setup(LED_FADE_PIN);
    led_fade_restart(1023, 600);        // Brightness, fade_duration                

    ws2812_setup(WS2812_PIN);

    app_console_setup();

    while (1) {
        uint64_t current_time = esp_timer_get_time();
        // mod_audio_test();

        // app_serial_i2c_task(current_time);

        if (current_time - interval_ref2 > 200000) {
            interval_ref2 = current_time;
            // mod_adc_1read(&mic_adc);
            // mod_adc_1read(&pir_adc);
            // printf("value = %u\n", pir_adc.value);
    
            // uint8_t value = map_value(mic_adc.value, 1910, 2000, 0, 64);
            // printf("raw = %u, value = %u\n", mic_adc.value, value);
            
            // printf("mic reading: %u\n", mic_adc.raw);
            // mod_adc_continous_read(&continous_read);
        }


        // if (current_time - interval_ref > pdMS_TO_TICKS(1)) {
        //     interval_ref = current_time;
        //     adc_read_arr[adc_read_index++] = single_adc.raw;

        //     if (adc_read_index >= MAX_ADC_SAMPLE) {
        //         adc_read_index = 0;
        //         data_output.data = (uint16_t*)adc_read_arr;
        //         // app_network_push_data(data_output);
        //     }
        // }

        #if WIFI_ENABLED
            app_network_task(current_time);
        #endif
        
        #if CONFIG_IDF_TARGET_ESP32C3
            cdc_read_task();
        #endif
            
        // gpio_digital_loop(current_time);
        led_fade_loop(current_time);

        // button_click_loop(current_time);
        // rotary_loop(current_time);

        ws2812_loop(current_time);
        app_console_task();

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
