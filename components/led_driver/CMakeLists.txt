idf_component_register(SRCS "led_toggle.c" "led_fade.c"
                       INCLUDE_DIRS "."
                       REQUIRES driver esp_timer)