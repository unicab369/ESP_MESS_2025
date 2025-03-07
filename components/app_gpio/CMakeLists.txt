idf_component_register(SRCS 
                            "led_toggle.c"
                            "led_fade.c"
                            "app_gpio.c"
                        INCLUDE_DIRS "."
                        REQUIRES
                            driver
                            esp_timer
                            ztimer_helper
                        )