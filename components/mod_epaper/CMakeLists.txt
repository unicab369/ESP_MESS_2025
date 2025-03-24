idf_component_register(SRCS
                        "mod_epaper.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        mod_spi
                        driver
                    REQUIRES)