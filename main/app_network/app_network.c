
#include "app_network.h"

#include "util_shared.h"
#include "esp_log.h"
#include "esp_err.h"

// modules
#include "mod_sd.h"
#include "display/display.h"

// networking
#include "mod_wifi.h"
#include "mod_wifi_nan.h"
#include "mod_espnow.h"
#include "ntp/ntp.h"
#include "http/http.h"
#include "esp_wifi.h"

#include "udp_socket/udp_socket.h"
#include "tcp_socket/tcp_socket.h"
#include "web_socket/web_socket.h"
// #include "modbus/modbus.h"

#include "sdkconfig.h"

static const char *TAG = "APP_NETWORK";


static void espnow_message_handler(espnow_received_message_t received_message) {
    ESP_LOGW(TAG,"received data:");
    ESP_LOGI(TAG, "Source Address: %02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(received_message.src_addr));

    ESP_LOGI(TAG, "rssi: %d", received_message.rssi);
    ESP_LOGI(TAG, "channel: %d", received_message.channel);
    ESP_LOGI(TAG, "group_id: %d", received_message.message->group_id);
    ESP_LOGI(TAG, "msg_id: %d", received_message.message->msg_id);
    ESP_LOGI(TAG, "access_code: %u", received_message.message->access_code);
    ESP_LOGI(TAG, "Data: %.*s", sizeof(received_message.message->data), received_message.message->data);
} 

void espnow_controller_send() {
    uint8_t dest_mac[6] = { 0xAA };
    uint8_t data[32] = { 0xBB };

    espnow_message_t message = {
        .access_code = 33,
        .group_id = 11,
        .msg_id = 12,
        .time_to_live = 15,
    };

    memcpy(message.target_addr, dest_mac, sizeof(message.target_addr));
    memcpy(message.data, data, sizeof(message.data));
    espnow_send((uint8_t*)&message, sizeof(message));
}

static uint8_t esp_mac[6];

app_wifi_interface_t interface = {
    .on_display_print = display_print_str,
    .max_retries = 10,
};

typedef struct {
    void(*on_dial_change)(int16_t value, uint8_t direction);
    void(*on_button_change)(uint8_t type, uint64_t duration);
    void(*on_gpio_read)(uint8_t pin, uint64_t value);
    
} control_interface_t;

void app_network_setup() {
    wifi_setup(&interface);

    wifi_softAp_begin(CONFIG_AP_WIFI_SSID, CONFIG_AP_WIFI_PASSWORD, 1);
    wifi_sta_begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);

    http_setup(&(http_interface_t){
        .on_file_fopen_cb = mod_sd_fopen,
        .on_file_fread_cb = mod_sd_fread,
        .on_file_fclose_cb = mod_sd_fclose,
        .on_display_print = display_print_str
    });
    
    wifi_connect();
    wifi_wps_begin();
    espnow_setup(esp_mac, espnow_message_handler);

    char mac_str[32];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(esp_mac));
    ESP_LOGW(TAG, "ESP mac: %s\n", mac_str);
    display_print_str(mac_str, 0);

    // wifi_scan();
    // wifi_nan_subscribe();
    // wifi_nan_publish();
}

static uint64_t second_interval_check = 0;

void app_network_task(uint64_t current_time) {
    if (current_time - second_interval_check > 1000000) {
        second_interval_check = current_time;

    } else {

    }

    // wifi_nan_checkPeers(current_time);
    // wifi_nan_sendData(current_time);

    wifi_event_t status = wifi_get_status();
    
    if (status == WIFI_EVENT_STA_CONNECTED) {
        // ntp_status_t ntp_status = ntp_task(current_time);
        web_socket_setup();
        // udp_server_socket_setup(current_time);
        // tcp_server_socket_setup(current_time);
        
    } else if (status == WIFI_EVENT_WIFI_READY) {
        web_socket_poll(current_time);

        // udp_server_socket_task();
        // udp_client_socket_send(current_time);

        // tcp_server_socket_task(current_time);
        // tcp_client_socket_task(current_time);
    }
}


void app_network_push_data(data_output_t data_output) {
    if (data_output.data == NULL) return;

    // Since buff is uint16_t, we need to allocate space for the type (1 byte) and the data
    uint16_t buff[1 + data_output.len]; // 1 for type (stored in the first 2 bytes), plus data

    // Store the type in the first 2 bytes (as uint16_t)
    buff[0] = (uint16_t)data_output.type;

    // Copy the data into the buffer starting from the second uint16_t element
    memcpy(buff + 1, data_output.data, data_output.len * sizeof(uint16_t));

    // send the message
    send_cur_websocket_message(buff, sizeof(buff));
}