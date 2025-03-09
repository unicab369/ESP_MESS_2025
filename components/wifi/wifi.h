#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum __attribute__((packed)) {
    WIFI_STATUS_INITIATED = 0x01,
    WIFI_STATUS_RETRY = 0x02,
    WIFI_STATUS_CONNECTED = 0x03,
    WIFI_STATUS_DISCONNECTED = 0x04,
    WIFI_STATUS_FAILED = 0x05,
} wifi_status_t;

typedef struct {
    uint8_t max_retries;
} app_wifi_config_t;


void wifi_setup(app_wifi_config_t *config);
void wifi_configure_softAp(const char* ap_ssid, const char* ap_passwd, uint8_t channel);
void wifi_configure_sta(const char* sta_ssid, const char* sta_passwd);

void wifi_connect(void);
void wifi_scan(void);
wifi_status_t wifi_check_status(uint64_t current_time);

// void udp_server_task();
// void udp_send_message(uint64_t current_time);

#endif