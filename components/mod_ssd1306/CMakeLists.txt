idf_component_register(SRCS
                        "mod_ssd1306.c"
                        "ssd1306_plot.c"
                        "ssd1306_segment.c"
                        "ssd1306_bitmap.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        driver
                        esp_timer
                        mod_i2c
                        mod_utility
                    REQUIRES)