idf_component_register(SRCS 
                        "mod_nimBLE.c"
                        "mod_BLEGap.c"
                        "mod_BLEGatt.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES bt)