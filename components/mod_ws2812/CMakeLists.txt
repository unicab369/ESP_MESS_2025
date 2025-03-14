idf_component_register(SRCS
                              "mod_ws2812.c"
                              "color_helper.c"
                         INCLUDE_DIRS "."
                         PRIV_REQUIRES
                              esp_driver_rmt
                              mod_utility
                         REQUIRES
                              driver
                              esp_timer
                         )