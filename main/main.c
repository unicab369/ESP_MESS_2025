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

static const char *TAG = "MAIN";
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
    ESP_LOGI(TAG, "IM HERE");
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


// #include "mod_lora.h"


#include "sx127x/esp_utils.h"
#include "sx127x/sx127x.c"

sx127x device;


#include "devices/spi/spi_devices.h"

#include "mod_spi.h"

void lora_rx_callback(sx127x *device, uint8_t *data, uint16_t data_length) {
    uint8_t payload[514];
    const char SYMBOLS[] = "0123456789ABCDEF";
    for (size_t i = 0; i < data_length; i++) {
        uint8_t cur = data[i];
        payload[2 * i] = SYMBOLS[cur >> 4];
        payload[2 * i + 1] = SYMBOLS[cur & 0x0F];
    }
    payload[data_length * 2] = '\0';

    int16_t rssi;
    ESP_ERROR_CHECK(sx127x_rx_get_packet_rssi(device, &rssi));
    float snr;
    ESP_ERROR_CHECK(sx127x_lora_rx_get_packet_snr(device, &snr));
    int32_t frequency_error;
    ESP_ERROR_CHECK(sx127x_rx_get_frequency_error(device, &frequency_error));
    ESP_LOGI(TAG, "received: %d %s rssi: %d snr: %f freq_error: %" PRId32, data_length, payload, rssi, snr, frequency_error);
}

void cad_callback(sx127x *device, int cad_detected) {
    if (cad_detected == 0) {
        ESP_LOGI(TAG, "cad not detected");
        ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_CAD, SX127x_MODULATION_LORA, device));
        return;
    }
    // put into RX mode first to handle interrupt as soon as possible
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, device));
    ESP_LOGI(TAG, "cad detected\n");
}

void check_dio0() {
    while (1) {
        if (gpio_get_level(DIO0)) sx127x_handle_interrupt(&device);

        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent busy-waiting
    }
}

void setup_loRa() {
    mod_spi_setup_rst(RST);
    ESP_LOGI(TAG, "sx127x was reset");

    //# Init SPI1 peripherals
    M_Spi_Conf spi_conf = {
        .host = 1,
        .mosi = MOSI,
        .miso = MISO,
        .clk = SCK,
        .cs = SS,
    };
    mod_spi_init(&spi_conf, 4E6);

    ESP_ERROR_CHECK(sx127x_create(spi_conf.spi_handle, &device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, SX127x_MODULATION_LORA, &device));
    ESP_ERROR_CHECK(sx127x_set_frequency(915000000, &device));  // 915MHz

    ESP_ERROR_CHECK(sx127x_lora_set_bandwidth(SX127x_BW_125000, &device));
    ESP_ERROR_CHECK(sx127x_lora_set_implicit_header(NULL, &device));
    ESP_ERROR_CHECK(sx127x_lora_set_modem_config_2(SX127x_SF_7, &device));
    ESP_ERROR_CHECK(sx127x_lora_set_syncword(18, &device));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, &device));
    ESP_ERROR_CHECK(sx127x_lora_reset_fifo(&device));
}

void app_main3() {
    setup_loRa();

    ESP_ERROR_CHECK(sx127x_rx_set_lna_boost_hf(true, &device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, SX127x_MODULATION_LORA, &device));
    ESP_ERROR_CHECK(sx127x_rx_set_lna_gain(SX127x_LNA_GAIN_G4, &device));

    sx127x_rx_set_callback(lora_rx_callback, &device);
    sx127x_lora_cad_set_callback(cad_callback, &device);

    // Setup DIO0 as input
    gpio_set_direction(DIO0, GPIO_MODE_INPUT);
    gpio_pulldown_en(DIO0);
    gpio_pullup_dis(DIO0);

    // Create task to monitor DIO0
    xTaskCreate(check_dio0, "dio0_monitor", 4096, &device, 2, NULL);
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, &device));
}


int messages_sent = 0;
int supported_power_levels[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20};
int supported_power_levels_count = sizeof(supported_power_levels) / sizeof(int);
int current_power_level = 0;

void tx_callback(sx127x *device) {
    if (messages_sent > 0) {
        ESP_LOGI(TAG, "transmitted");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (messages_sent == 0) {
        uint8_t data[] = {0xCA, 0xFE};
        ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data, sizeof(data), device));
    } else if (messages_sent == 1) {
        // 200 bytes
        uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7};
        ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data, sizeof(data), device));
    } else if (messages_sent == 2) {
        // 255 bytes
        uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe};
        ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data, sizeof(data), device));
    } else if (current_power_level < supported_power_levels_count) {
        uint8_t data[] = {0xCA, 0xFE};
        ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data, sizeof(data), device));
        ESP_ERROR_CHECK(sx127x_tx_set_pa_config(SX127x_PA_PIN_BOOST, supported_power_levels[current_power_level], device));
        current_power_level++;
    } else {
        return;
    }
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, SX127x_MODULATION_LORA, device));
    ESP_LOGI(TAG, "transmitting");
    messages_sent++;
}

void app_main() {
    setup_loRa();

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, SX127x_MODULATION_LORA, &device));

    sx127x_tx_set_callback(tx_callback, &device);

    gpio_install_isr_service(0);
    setup_gpio_interrupts((gpio_num_t)DIO0, &device, GPIO_INTR_POSEDGE);

    ESP_ERROR_CHECK(sx127x_tx_set_pa_config(SX127x_PA_PIN_BOOST, supported_power_levels[current_power_level], &device));
    sx127x_tx_header_t header = {
        .enable_crc = true,
        .coding_rate = SX127x_CR_4_5};
    ESP_ERROR_CHECK(sx127x_lora_tx_set_explicit_header(&header, &device));

    ESP_ERROR_CHECK(setup_tx_task(&device, tx_callback));
}




void app_main2(void) {
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

    // //# Init SPI2 peripherals
    // M_Spi_Conf spi_config_b = {
    //     .host = 2,
    //     .mosi = SPI2_MOSI,
    //     .miso = SPI2_MISO,
    //     .clk = SPI2_CLK,
    //     .cs = SPI2_CS0,
    // };

    // ret = mod_spi_init(&spi_config_b, 20E6);
    // if (ret == ESP_OK) {
    //     // spi_setup_sdCard(spi_config_b.host, spi_config_b.cs);

    //     //! DC and RST pins are required for ST7735
    //     spi_config_b.dc = SPI_DC;
    //     spi_config_b.rst = SPI_RST;

    //     //! Reset the device first
    //     // mod_spi_setup_rst(SPI_RST);
    //     // spi_setup_st7735(&spi_config_b, SPI_CS_X1);

    //     // mod_spi_setup_rst(SPI_RST);
    //     // setup_loRa(&spi_config_b.spi_handle);
    // }


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
