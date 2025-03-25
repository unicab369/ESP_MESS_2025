#include "main.h"

#include "mod_utility.h"
#include "mod_adc.h"
#include "mod_sd.h"
#include "mod_mbedtls.h"
#include "mod_ws2812.h"
#include "mod_button.h"
#include "mod_rotary.h"
#include "mod_ssd1306.h"
#include "ssd1306_plot.h"
#include "ssd1306_segment.h"
#include "ssd1306_bitmap.h"

#include "devices/i2c/sensors.h"
#include "mod_i2s.h"


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
    ssd1306_set_printMode(direction);
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

char display_buff[64];

void on_resolve_bh1750(float lux) {
    snprintf(display_buff, sizeof(display_buff), "BH1750 %.2f", lux);
    ssd1306_print_str(display_buff, 2);
}

void on_resolve_ap3216(uint16_t ps, uint16_t als) {
    snprintf(display_buff, sizeof(display_buff), "prox %u, als %u", ps, als);
    ssd1306_print_str(display_buff, 5);
}

void on_resolve_apds9960(uint8_t prox, uint16_t clear,
    uint16_t red, uint16_t green, uint16_t blue) {
    snprintf(display_buff, sizeof(display_buff), "ps %u, w %u, r %u, g %u, b %u", prox, clear, red, green, blue);
    ssd1306_print_str(display_buff, 6);
}

void on_resolve_max4400(float lux) {
    snprintf(display_buff, sizeof(display_buff), "lux %.2f", lux);
    // ssd1306_print_str(display_buff, 7);
}

void on_resolve_vl53lox(uint8_t distance) {
    snprintf(display_buff, sizeof(display_buff), "dist: %u", distance);
    // ssd1306_print_str(display_buff, 7);
}

void on_resolve_mpu6050(int16_t accel_x, int16_t accel_y, int16_t accel_z) {
    snprintf(display_buff, sizeof(display_buff), "x %u, y %u, z %u", accel_x, accel_y, accel_z);
    ssd1306_print_str(display_buff, 4);
}

void on_resolve_ina219(int16_t shunt, int16_t bus_mV, int16_t current, int16_t power) {
    snprintf(display_buff, sizeof(display_buff),"sh %hu, bus %hu, cur %hd, p %hu", 
                shunt, bus_mV, current, power);
    ssd1306_print_str(display_buff, 7);
}

void on_resolve_ds3231(ds3231_dateTime_t *datetime) {
    snprintf(display_buff, sizeof(display_buff), "%u/%u/%u %u:%u:%u", 
            datetime->month, datetime->date, datetime->year,
            datetime->hr, datetime->min, datetime->sec);
    ssd1306_print_str(display_buff, 1);
}

void on_resolve_sht31(float temp, float hum) {
    snprintf(display_buff, sizeof(display_buff), "Temp %.2f, hum %.2f", temp, hum);
    ssd1306_print_str(display_buff, 3);
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

    display_setup(SCL_PIN, SDA_PIN);

    M_Sensors_Interface sensors_intf = {
        .on_handle_bh1750 = on_resolve_bh1750,
        .on_handle_ap3216 = on_resolve_ap3216,
        .on_handle_apds9960 = on_resolve_apds9960,
        .on_handle_max4400 = on_resolve_max4400,
        .on_handle_vl53lox = on_resolve_vl53lox,
        .on_handle_mpu6050 = on_resolve_mpu6050,
        .on_handle_ina219 = on_resolve_ina219,
        .on_handle_ds3231 = on_resolve_ds3231,
        .on_handle_sht31 = on_resolve_sht31

    };
    i2c_sensors_setup(&sensors_intf);
    
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

    storage_sd_config_t sd_config = {
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
    };

    mod_sd_spi_config(&sd_config);
    mod_sd_test();

    M_Spi_Conf spi3_conf = {
        .host = SPI3_HOST,
        .mosi = SPI2_MOSI,
        .clk = SPI2_SCLK,
        .dc = SPI2_DC
    };

    ret = mod_spi_init(&spi3_conf);
    if (ret == ESP_OK) {
        display_spi_setup(SPI2_RES, SPI2_BUSY, &spi3_conf);
    }
    

    //! Audio test
    // mod_audio_setup(SPI2_BUSY);

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

    adc_single_read_t mic_adc = {
        .gpio = MIC_IN,
    };
    mod_adc_1read_setup(&mic_adc);

    adc_single_read_t pir_adc = {
        .gpio = PIR_IN,
    };
    mod_adc_1read_setup(&pir_adc);


    // adc_continous_read_t continous_read = (adc_continous_read_t){
    //     .unit = ADC_UNIT_1
    // };

    // mod_adc_continous_setup(&continous_read);
    
    // mod_i2s_setup();
    // mod_i2s_send();


    uint64_t interval_ref = 0;
    uint64_t interval_ref2 = 0;

    #include "mod_nimBLE.h"
    mod_nimbleBLE_setup(false);
    xTaskCreate(mod_nimbleBLE_task, "NimBLE Host", 4*1024, NULL, 5, NULL);

    while (1) {
        uint64_t current_time = esp_timer_get_time();
            // mod_audio_test();

        i2c_sensor_readings(current_time);

        if (current_time - interval_ref2 > 200000) {
            interval_ref2 = current_time;
            mod_adc_1read(&mic_adc);
            mod_adc_1read(&pir_adc);
            // printf("value = %u\n", pir_adc.value);

            uint8_t value = map_value(mic_adc.value, 1910, 2000, 0, 64);
            // printf("raw = %u, value = %u\n", mic_adc.value, value);

            if (ssd1306_print_mode == 2) {
                ssd1306_spectrum(5);
            }
            else if (ssd1306_print_mode == 3) {
                ssd1306_test_digits();
            }
            else if (ssd1306_print_mode == 0) {
                ssd1306_test_bitmaps();
            }
            
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

        button_click_loop(current_time);
        rotary_loop(current_time);

        ws2812_loop(current_time);
        app_console_task();

        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
