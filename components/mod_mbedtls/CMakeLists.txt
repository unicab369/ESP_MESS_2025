idf_component_register(SRCS "mod_mbedtls.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        mbedtls
                        esp_timer
                    )