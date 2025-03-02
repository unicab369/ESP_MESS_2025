idf_component_register(SRCS "timer_pulse.c"
                         INCLUDE_DIRS "."
                         REQUIRES
                              driver
                              esp_timer
                         )