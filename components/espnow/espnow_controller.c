#include "espnow_controller.h"
#include "espnow_driver.h"
#include <string.h>
#include "esp_timer.h"
#include "esp_log.h"

void espnow_message_handler(const uint8_t *data, int len) {
    ESP_LOGW("TAG","received data : %.*s", len, data);

    if (len < sizeof(espnow_send_message_t)) {
        ESP_LOGE("TAG", "Received data size mismatch. Expected at least %d bytes, got %d", sizeof(espnow_send_message_t), len);
        return;
    }

    const espnow_send_message_t *received_message = (const espnow_send_message_t *)data;
    ESP_LOGI("TAG", "Received message:");
    ESP_LOGI("TAG", "Group ID: %d", received_message->group_id);
    ESP_LOGI("TAG", "Message ID: %d", received_message->msg_id);
    ESP_LOGI("TAG", "Access Code: %d", received_message->access_code);
    ESP_LOGI("TAG", "Hops: %d", received_message->hops);
    ESP_LOGI("TAG", "Time to Live: %d", received_message->time_to_live);
    ESP_LOGI("TAG", "Sender Time: %lld", received_message->sender_time);
    ESP_LOGI("TAG", "Data: %.*s", len - offsetof(espnow_send_message_t, data), received_message->data);
}

void espnow_controller_setup() {
    espnow_setup(espnow_message_handler);
}

void espnow_controller_send() {
    uint8_t dest_mac[6] = { 0xAA };
    uint8_t data[32] = { 0xBB };

    espnow_send_message_t message = {
        .group_id = 1,
        .msg_id = 2,
        .access_code = 3,
        .hops = 4,
        .time_to_live = 5,
        .sender_time = esp_timer_get_time(),
    };

    memcpy(message.dest_addr, dest_mac, sizeof(message.dest_addr));
    memcpy(message.data, data, sizeof(message.data));

    espnow_send((uint8_t*)&message, sizeof(message));
}