
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "driver/usb_serial_jtag.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "led_toggle.h"
#include "led_fade.h"
#include "uart_driver.h"
#include "behavior/behavior.h"
#include "timer_pulse.h"
#include "console/app_console.h"

#include "display/display.h"
#include "sdkconfig.h"


#define ESP32_BOARD_V3 true


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
        #define WS2812_PIN 2

        // SD-SPI: 3v3 | CS | MOSI | CLK | MISO | GND
        // SD_MMC: _ DO | GND | SCLK | VCC | GND |DI | CS | __

        #define SPI_MISO 19
        #define SPI_MOSI 23
        #define SPI_CLK 18
        #define SPI_CS 26

        // #define SPI2_MISO 4
        #define SPI2_MOSI 12        // SDA
        #define SPI2_SCLK 14
        #define SPI2_DC 17          // DC - AO
        #define SPI2_RES 27

        #define MMC_D0 SPI_MISO
        #define MMC_DI SPI_MOSI
        #define MMC_CLK SPI_CLK
        #define MMC_CS SPI_CS
        
        #define SCL_PIN 32
        #define SDA_PIN 33

        #define GPIO_34 34
        #define MIC_IN 35
        #define PIR_IN 36

        // GPIO 39

    #else
        #define LED_FADE_PIN 22
        #define BLINK_PIN 5
        #define BUTTON_PIN 23
        #define WS2812_PIN 12
    #endif
#endif