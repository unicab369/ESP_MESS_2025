idf_component_register(SRCS 
                            "led_toggle.c"
                            "led_fade.c"
                            "mod_gpio.c"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES
                            mod_utility
                        REQUIRES
                            driver
                            esp_timer
                        )