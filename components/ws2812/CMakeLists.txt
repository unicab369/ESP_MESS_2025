idf_component_register(SRCS "ws2812.c"
                         INCLUDE_DIRS "."
                         PRIV_REQUIRES
                              esp_driver_rmt
                         REQUIRES
                              driver
                              esp_timer
                              ztimer_helper
                         )