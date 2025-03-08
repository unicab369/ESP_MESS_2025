idf_component_register(SRCS "app_i2c.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                        driver
                        esp_timer
                    )