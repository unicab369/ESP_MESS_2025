idf_component_register(SRCS
                              "wifi.c"
                              "ntp/ntp.c"
                              "udp_socket/udp_socket.c"
                              "tcp_socket/tcp_socket.c"
                              "http/http.c"
                         INCLUDE_DIRS "."
                         PRIV_REQUIRES
                              esp_wifi
                              esp-tls
                              esp_eth
                              esp_http_server
                              esp_http_client
                         REQUIRES
                              driver
                              esp_timer
                         )