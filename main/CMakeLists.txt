idf_component_register(SRCS "main.c"
                            "littlefs/littlefs.c"
                            "behavior/behavior.c"
                            "console/base_console.c"
                            "console/app_console.c"
                            "console/cmd_wifi/cmd_wifi.c"
                            "console/cmd_nvs/cmd_nvs.c"
                            "console/cmd_system/cmd_system.c"
                            "console/cmd_system/cmd_system_sleep.c"
                            "modbus/modbus.c"
                            "display/display.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        esp_driver_rmt
                        wifi
                        wifi_nan
                        nvs_flash
                        console
                        spi_flash
                        storage
                        app_mbedtls
                        app_ssd1306
                    REQUIRES
                        driver
                        app_gpio
                        button_driver
                        rotary_driver
                        uart_driver
                        cdc_driver
                        espnow
                        ws2812
                        ztimer_helper
                    )

# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
littlefs_create_partition_image(storage ../flash_data FLASH_IN_PROJECT)