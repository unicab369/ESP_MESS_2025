#include "espnow_controller.h"
#include "espnow_driver.h"
#include <string.h>
#include "esp_timer.h"
#include "esp_log.h"

void espnow_message_handler(espnow_received_message_t received_message) {
    ESP_LOGW("TAG","received data");
    ESP_LOGI("TAG", "rssi: %d", received_message.rssi);
    ESP_LOGI("TAG", "channel: %d", received_message.channel);
    ESP_LOGI("TAG", "group_id: %d", received_message.message->group_id);
    ESP_LOGI("TAG", "msg_id: %d", received_message.message->msg_id);
    ESP_LOGI("TAG", "access_code: %u", received_message.message->access_code);
    ESP_LOGI("TAG", "Data: %.*s", sizeof(received_message.message->data), received_message.message->data);
}

    
void espnow_controller_setup() {
    espnow_setup(espnow_message_handler);
}

void espnow_controller_send() {
    uint8_t dest_mac[6] = { 0xAA };
    uint8_t data[32] = { 0xBB };

    espnow_message_t message = {
        .access_code = 3,
        .group_id = 1,
        .msg_id = 2,
        .time_to_live = 5,
    };

    memcpy(message.target_addr, dest_mac, sizeof(message.target_addr));
    memcpy(message.data, data, sizeof(message.data));

    espnow_send((uint8_t*)&message, sizeof(message));
}