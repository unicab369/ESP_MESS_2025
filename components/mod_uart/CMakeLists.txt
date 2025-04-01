idf_component_register(SRCS "mod_uart.c"
                        INCLUDE_DIRS "."
                        REQUIRES
                            driver
                            esp_timer
                        )