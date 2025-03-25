idf_component_register(SRCS 
                        "mod_nimBLE_beacon.c"
                        "mod_nimBLE_service.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES bt)