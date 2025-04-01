#include "main.h"

#include "mod_utility.h"
#include "mod_adc.h"

#include "mod_mbedtls.h"
// #include "mod_ws2812.h"
#include "mod_button.h"
#include "mod_rotary.h"

#include "mod_i2s.h"
#include "devices/app_serial.h"

#include "driver/spi_master.h"

#include "devices/spi/spi_devices.h"

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

    // app_serial_i2c_setup(SCL_PIN, SDA_PIN, 0);
    // app_serial_i2c_setup(SCL_PIN2, SDA_PIN2, 1);
    
    // #if WIFI_ENABLED
    //     app_network_setup();

    //     http_setup(&(http_interface_t){
    //         .on_file_fopen_cb   = mod_sd_fopen,
    //         .on_file_fread_cb   = mod_sd_fread,
    //         .on_file_fclose_cb  = mod_sd_fclose,
    //         .on_display_print   = display_print_str,
    //         .on_request_data    = http_request_handler
    //     });
    // #endif

    // //# Init SPI1 peripherals
    // M_Spi_Conf spi_config_a = {
    //     .host = 1,
    //     .mosi = SPI_MOSI,
    //     .miso = SPI_MISO,
    //     .clk = SPI_CLK,
    //     .cs = SPI_CS0,
    // };

    // ret = mod_spi_init(&spi_config_a, 20E6);
    // if (ret == ESP_OK) {
    //     spi_setup_sdCard(spi_config_a.host, spi_config_a.cs);

    //     //! DC and RST pins are required for ST7735
    //     spi_config_a.dc = SPI_DC;
    //     spi_config_a.rst = SPI_RST;

    //     //! Reset the device first
    //     mod_spi_setup_rst(SPI_RST);
    //     spi_setup_st7735(&spi_config_a, SPI_CS_X0);
    // }

    // //# Init SPI2 peripherals
    // M_Spi_Conf spi_config_b = {
    //     .host = 2,
    //     .mosi = SPI2_MOSI,
    //     .miso = SPI2_MISO,
    //     .clk = SPI2_CLK,
    //     .cs = SPI2_CS0,
    // };

    // //# Init SPI1 peripherals
    // // M_Spi_Conf spi_config_b = {
    // //     .host = 1,
    // //     .mosi = 23,
    // //     .miso = 19,
    // //     .clk = 18,
    // //     .cs = 5,
    // // };

    // ret = mod_spi_init(&spi_config_b, 20E6);        //! 4E6 for LoRa

    // if (ret == ESP_OK) {
    //     // spi_setup_sdCard(spi_config_b.host, spi_config_b.cs);

    //     //! DC and RST pins are required for ST7735
    //     spi_config_b.dc = SPI_DC;
    //     spi_config_b.rst = SPI_RST;

    //     //! Reset the device first
    //     mod_spi_setup_rst(SPI_RST);
    //     spi_setup_st7735(&spi_config_b, SPI_CS_X1);

    //     // mod_spi_setup_rst(SPI_RST);

    //     // mod_sx127_listen(&spi_config_b);
    //     // mod_sx127_send(&spi_config_b);
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


    // mod_mbedtls_setup();
    
    // data_output_t data_output = {
    //     .type = DATA_OUTPUT_ANALOG,
    //     .len = 100
    // };

    
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
    
    // #include "devices/gpio/app_gpio.h"

    // app_gpio_set_adc();
    
    // app_gpio_ws2812(WS2812_PIN);

    led_fade_setup(LED_FADE_PIN);
    led_fade_restart(1023, 600);        // Brightness, fade_duration                

    app_console_setup();

    while (1) {
        uint64_t current_time = esp_timer_get_time();
        // mod_audio_test();

        app_serial_i2c_task(current_time);

        #if WIFI_ENABLED
            app_network_task(current_time);
        #endif
        
        #if CONFIG_IDF_TARGET_ESP32C3
            cdc_read_task();
        #endif
            
        // gpio_digital_loop(current_time);
        led_fade_loop(current_time);

        // button_click_loop(current_time);
        // // rotary_loop(current_time);

        // app_gpio_task(current_time);
        // app_console_task();

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
