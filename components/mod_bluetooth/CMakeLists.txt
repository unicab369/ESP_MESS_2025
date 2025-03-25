idf_component_register(SRCS 
                        "mod_nimBLE.c"
                        "mod_BLEScan.c"
                        "BLEGap/mod_BLEGap.c"
                        "BLEGatt/mod_BLEGatt.c"
                        "GattHtp/mod_GattHtp.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES bt nimble_central_utils)