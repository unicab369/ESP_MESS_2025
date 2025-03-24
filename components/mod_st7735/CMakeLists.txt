idf_component_register(SRCS 
                         "mod_st7735.c"
                         "st7735_shape.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                         driver
                         esp_timer
                         mod_spi
                         mod_utility
                    )