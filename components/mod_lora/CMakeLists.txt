idf_component_register(SRCS "mod_lora.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        driver
                        mod_spi
                    )