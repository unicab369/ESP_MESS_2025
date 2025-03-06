#include "espnow_driver.h"
#include <string.h>
#include "esp_timer.h"
#include "esp_log.h"

#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

#define ESPNOW_MAXDELAY 512
#define ESPNOW_CHANNEL 1
#define CONFIG_ESPNOW_PMK "pmk1234567890123"
#define ESPNOW_QUEUE_SIZE           6

static const char *TAG = "ESP-NOW";

#define BROADCAST_ADDRESS {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}
static const uint8_t broadcast_mac[] = BROADCAST_ADDRESS;

static uint64_t last_update_time = 0;
static uint64_t current_time;
static espnow_message_cb message_callback = NULL;

static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if (message_callback == NULL) return;
    if (len < sizeof(espnow_message_t)) {
        ESP_LOGE(TAG, "Received data size mismatch. Expected at least %d bytes, got %d", sizeof(espnow_message_t), len);
        return;
    }

    espnow_received_message_t received_message = {
        .rssi = recv_info->rx_ctrl->rssi,
        .channel = recv_info->rx_ctrl->channel,
        .message =  (espnow_message_t *)data
    };
    // received_message.message = (espnow_message_t *)data;
    memcpy(received_message.src_addr, recv_info->src_addr, sizeof(received_message.src_addr));

    message_callback(received_message);
}

esp_err_t espnow_setup(uint8_t* esp_mac, espnow_message_cb callback)
{
    message_callback = callback;
    esp_read_mac(esp_mac, 6);       //! ORDER DOES MATTER: after wifi_setup()

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    esp_now_peer_info_t peerInfo = {
        .channel = 0,
        .encrypt = false,
    };
    memcpy(peerInfo.peer_addr, broadcast_mac, 6); // Copy address
    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));

    current_time = esp_timer_get_time();
    return ESP_OK;
}

esp_err_t espnow_send(uint8_t* data, size_t len) {
    current_time = esp_timer_get_time();
    if (current_time - last_update_time < 2000000) return ESP_OK;
    last_update_time = current_time;

    esp_err_t err = esp_now_send(broadcast_mac, data, len);
    // esp_err_t err = esp_now_send(broadcast_mac, (uint8_t *) "Sending via ESP-NOW", strlen("Sending via ESP-NOW"));
    ESP_LOGI(TAG,"esp now status : %s", esp_err_to_name(err));
    return err;
}