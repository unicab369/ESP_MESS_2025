idf_component_register(SRCS 
                            "mod_espnow.c" 
                      INCLUDE_DIRS "."
                      REQUIRES
                      driver
                      esp_timer
                      nvs_flash
                      esp_wifi)