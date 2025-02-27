idf_component_register(SRCS "main.c"
                            "my_littlefs.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        driver
                        led_driver
                        button_driver
                        rotary_driver
                        uart_driver
                        cdc_driver
                        espnow
                        littlefs_driver
                    )

# Note: you must have a partition named the first argument (here it's "littlefs")
# in your partition table csv file.
littlefs_create_partition_image(storage ../flash_data FLASH_IN_PROJECT)