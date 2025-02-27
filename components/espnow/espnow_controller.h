#ifndef ESPNOW_CONTROLLER_H
#define ESPNOW_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>
#include <esp_err.h> 

typedef struct {
    uint8_t src_addr[6];
    uint8_t dest_addr[6];
    uint8_t group_id;
    uint8_t msg_id;
    uint16_t access_code;

    uint8_t hops;
    uint8_t time_to_live;
    uint64_t sender_time;
    uint8_t data[32];
} espnow_receive_message_t;

typedef struct {
    uint8_t dest_addr[6];
    uint8_t group_id;
    uint8_t msg_id;
    uint16_t access_code;

    uint8_t hops;
    uint8_t time_to_live;
    uint64_t sender_time;
    uint8_t data[32];
} espnow_send_message_t;

// Function prototypes
void espnow_controller_setup(void);
void espnow_controller_send(void);

#endif