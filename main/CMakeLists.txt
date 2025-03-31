idf_component_register(SRCS "main.c"
                            "littlefs/littlefs.c"
                            "behavior/behavior.c"
                            "console/base_console.c"
                            "console/app_console.c"
                            "console/cmd_wifi/cmd_wifi.c"
                            "console/cmd_nvs/cmd_nvs.c"
                            "console/cmd_system/cmd_system.c"
                            "console/cmd_system/cmd_system_sleep.c"
                            "console/cmd_espnow/cmd_espnow.c"
                            "app_network/app_network.c"
                            "modbus/modbus.c"
                            "devices/i2c/sensors.c"
                            "devices/spi/spi_devices.c"
                            "devices/app_serial.c"

                            "sx127x/esp_utils.c"
                            "sx127x/sx127x_esp_spi.c"
                            "sx127x/sx127x.c"
                    INCLUDE_DIRS 
                        "."

                    PRIV_REQUIRES
                        esp_driver_rmt
                        nvs_flash
                        console
                        spi_flash
                        mod_wifi
                        mod_sd
                        mod_mbedtls
                        mod_ssd1306
                        mod_nvs
                        mod_espnow
                        mod_button
                        mod_rotary
                        mod_ws2812
                        mod_gpio
                        mod_adc
                        mod_utility
                        mod_i2c
                        mod_st7735
                        mod_epaper
                        mod_spi
                        mod_i2s
                        mod_audio
                        mod_bluetooth
                    REQUIRES
                        driver
                        uart_driver
                        cdc_driver
                        # sx127x
                    )

# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
littlefs_create_partition_image(storage ../flash_data FLASH_IN_PROJECT)