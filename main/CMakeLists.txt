idf_component_register(SRCS "main.c"
                            "littlefs/littlefs.c"
                            "behavior/behavior.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        esp_driver_rmt
                    REQUIRES
                        driver
                        led_driver
                        button_driver
                        rotary_driver
                        uart_driver
                        cdc_driver
                        espnow
                        ws2812
                    )

# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
littlefs_create_partition_image(storage ../flash_data FLASH_IN_PROJECT)