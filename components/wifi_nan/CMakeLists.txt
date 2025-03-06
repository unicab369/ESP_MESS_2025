idf_component_register(SRCS
                              "wifi_nan.c"
                         INCLUDE_DIRS "."
                         PRIV_REQUIRES
                              esp_wifi
                              nvs_flash
                         REQUIRES
                              driver
                              esp_timer
                         )