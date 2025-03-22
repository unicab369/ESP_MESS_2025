idf_component_register(SRCS 
                         "timer_pulse.c"
                         "cycle_sequence.c"
                         "mod_utility.c"
                         "mod_bitmap.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                         driver
                         esp_timer
                    )