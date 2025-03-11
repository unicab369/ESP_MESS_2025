
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dev_config.h"
#include "driver/ledc.h"
#include "driver/usb_serial_jtag.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "led_toggle.h"
#include "led_fade.h"
#include "mod_button.h"
#include "mod_rotary.h"
#include "uart_driver.h"
#include "behavior/behavior.h"
#include "mod_ws2812.h"
#include "timer_pulse.h"
#include "console/app_console.h"

#include "mod_sd.h"
#include "mod_mbedtls.h"
#include "display/display.h"
#include "sdkconfig.h"


#define ESP32_BOARD_V3 true

static const char *TAG = "MAIN";

#ifndef WIFI_SSID
#define WIFI_SSID "My_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "MY_PASSWORD"
#endif

#ifndef AP_WIFI_SSID
#define AP_WIFI_SSID "ESP_MESS_AP"
#endif

#ifndef AP_WIFI_PASSWORD
#define AP_WIFI_PASSWORD "password"
#endif

#if CONFIG_IDF_TARGET_ESP32C3
    #include "cdc_driver.h"
    #define LED_FADE_PIN 2
    #define BLINK_PIN 10
    #define BUTTON_PIN 9
#elif CONFIG_IDF_TARGET_ESP32
    #ifdef ESP32_BOARD_V3
        #define LED_FADE_PIN 22
        #define BUTTON_PIN 16
        #define ROTARY_CLK 15
        #define ROTARY_DT 13
        #define BUZZER_PIN 5
        #define WS2812_PIN 4

        // SD-SPI: 3v3 | CS | MOSI | CLK | MISO | GND
        // SD_MMC: _ DO | GND | SCLK | VCC | GND |DI | CS | __

        #define SPI_MISO 19
        #define SPI_MOSI 23
        #define SPI_CLK 18
        #define SPI_CS 26

        #define MMC_D0 SPI_MISO
        #define MMC_DI SPI_MOSI
        #define MMC_CLK SPI_CLK
        #define MMC_CS SPI_CS
        
        #define SCL_PIN 32
        #define SDA_PIN 33
        #define SSD_1306_ADDR 0x3C
    #else
        #define LED_FADE_PIN 22
        #define BLINK_PIN 5
        #define BUTTON_PIN 23
        #define WS2812_PIN 12
    #endif
#endif

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]