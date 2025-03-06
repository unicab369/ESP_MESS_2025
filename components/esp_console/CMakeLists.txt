idf_component_register(SRCS 
                            "esp_console.c" 
                      INCLUDE_DIRS "."
                      PRIV_REQUIRES 
                        console
                        esp_driver_uart
                        fatfs
                        esp_driver_usb_serial_jtag
                        nvs_flash
                        # cmd_system
                        # cmd_wifi
                        # cmd_nvs
                      )