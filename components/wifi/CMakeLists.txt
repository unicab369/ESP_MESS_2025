idf_component_register(SRCS
                              "wifi.c" "ntp.c"
                         INCLUDE_DIRS "."
                         PRIV_REQUIRES
                              esp_wifi
                         REQUIRES
                              driver
                              esp_timer
                         )