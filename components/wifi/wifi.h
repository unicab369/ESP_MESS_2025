#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Function prototypes
void wifi_setup(void);
void wifi_scan(void);
void wifi_check_status(uint64_t current_time);

void wifi_start_sta(const char* sta_ssid, const char* sta_passwd);
void udp_server_task();

#endif