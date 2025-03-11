idf_component_register(SRCS
                              "mod_ws2812.c"
                              "color_helper.c"
                         INCLUDE_DIRS "."
                         PRIV_REQUIRES
                              esp_driver_rmt
                         REQUIRES
                              driver
                              esp_timer
                              ztimer_helper
                         )