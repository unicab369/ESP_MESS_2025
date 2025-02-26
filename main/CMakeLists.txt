idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        driver
                        led_driver
                        button_driver
                        rotary_driver
                        uart_controller
                        cdc_driver
                        espNow_driver
                    )