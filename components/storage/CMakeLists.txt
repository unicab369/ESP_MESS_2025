idf_component_register(SRCS "storage_sd.c"
                        INCLUDE_DIRS "."
                        REQUIRES fatfs sd_card
                    )