#ifndef ESPNOW_DIRVER_H
#define ESPNOW_DIRVER_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>
#include <esp_err.h> 

typedef struct {
    uint8_t target_addr[6];
    uint8_t group_id;
    uint8_t msg_id;
    uint16_t access_code;
    uint8_t time_to_live;
    uint8_t data[32];
} espnow_message_t;

typedef struct {
    uint8_t src_addr[6];
    uint8_t rssi;
    uint8_t channel;
    espnow_message_t* message;
} espnow_received_message_t;

typedef void (*espnow_message_cb)(espnow_received_message_t received_message);

// Function prototypes
esp_err_t espnow_setup(uint8_t* esp_mac, espnow_message_cb callback);
esp_err_t espnow_send(uint8_t* data, size_t len);

#endif