#include "wifi.h"
#include <string.h>

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_net_stack.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "regex.h"
#include "esp_nan.h"
#include "esp_log.h"
#include "esp_err.h"
#include "ntp.h"
#include "udp.h"


/* STA Configuration */
#define EXAMPLE_ESP_MAXIMUM_RETRY           20

/*DHCP server option*/
#define DHCPS_OFFER_DNS             0x02

static const char *TAG_AP = "WiFi SoftAP";
static const char *TAG_STA = "WiFi Sta";
static int s_retry_num = 0;

static const char *TAG = "WIFI";

static uint64_t last_update_time = 0;
static uint8_t remaining_retries = 0;
static wifi_status_t current_status = WIFI_STATUS_DISCONNECTED;

void test_ntp(void);

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG_AP, "Station "MACSTR" joined, AID=%d",
                MAC2STR(event->mac), event->aid);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG_AP, "Station "MACSTR" left, AID=%d, reason:%d",
                MAC2STR(event->mac), event->aid, event->reason);

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // esp_wifi_connect();
        // ESP_LOGI(TAG_STA, "Station started");

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    }
}

void softap_set_dns_addr(esp_netif_t *esp_netif_ap,esp_netif_t *esp_netif_sta)
{
    esp_netif_dns_info_t dns;
    esp_netif_get_dns_info(esp_netif_sta,ESP_NETIF_DNS_MAIN,&dns);
    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));
}

app_wifi_config_t* wifi_config;

void wifi_setup(app_wifi_config_t *config)
{
    wifi_config = config;
    remaining_retries = config->max_retries;

    //! Minimum setup for ESP-NOW
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    // ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    // ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    // ESP_ERROR_CHECK( esp_wifi_start());
    // ESP_ERROR_CHECK( esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

    //! NOTE: these 2 lines need to start FIRST!
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &wifi_event_handler, NULL, NULL));

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // /* Set sta as the default interface */
    // esp_netif_set_default_netif(esp_netif_sta);

}

void wifi_setup_softAp(const char* ap_ssid, const char* ap_passwd, uint8_t channel) {
    /* Initialize AP */
    ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");

    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .channel = channel,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    // Copy the SSID and password into the configuration structure
    strncpy((char*)wifi_ap_config.ap.ssid, ap_ssid, sizeof(wifi_ap_config.ap.ssid));
    strncpy((char*)wifi_ap_config.ap.password, ap_passwd, sizeof(wifi_ap_config.ap.password));

    if (strlen(ap_passwd) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
            ap_ssid, ap_passwd, channel);

    // /* Enable napt on the AP netif */
    // if (esp_netif_napt_enable(esp_netif_ap) != ESP_OK) {
    //     ESP_LOGE(TAG_STA, "NAPT not enabled on the netif: %p", esp_netif_ap);
    // }
}

void wifi_setup_sta(const char* sta_ssid, const char* sta_passwd) {
    printf("connecting to %s\n", sta_ssid);

    /* Initialize STA */
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = EXAMPLE_ESP_MAXIMUM_RETRY,
            .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    // Copy the SSID and password into the configuration structure
    strncpy((char *)wifi_sta_config.sta.ssid, sta_ssid, sizeof(wifi_sta_config.sta.ssid));
    strncpy((char *)wifi_sta_config.sta.password, sta_passwd, sizeof(wifi_sta_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config) );
    ESP_LOGI(TAG_STA, "wifi_init_sta finished.");
}

void wifi_connect() {
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    current_status = WIFI_STATUS_INITIATED;
}

void wifi_disconnect() {
    esp_wifi_disconnect();
    current_status = WIFI_STATUS_DISCONNECTED;

    // esp_wifi_stop();     // stop driver
    // esp_wifi_deinit();
}

void wifi_stop() {
    esp_wifi_stop();
    // esp_wifi_deinit();
}


wifi_status_t wifi_check_status(uint64_t current_time) {
    if (current_status == WIFI_STATUS_CONNECTED) return current_status;
    if (current_time - last_update_time < 1000000) return current_status;
    last_update_time = current_time;

    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_STA, "Connected to AP: %s, RSSI: %d", ap_info.ssid, ap_info.rssi);
        current_status = WIFI_STATUS_CONNECTED;
    } else if (remaining_retries < wifi_config->max_retries) {
        ESP_LOGI(TAG_STA, "Retrying connection to AP: %s", esp_err_to_name(ret));
        remaining_retries = 0;
        current_status = WIFI_STATUS_RETRY;
    } else {
        ESP_LOGE(TAG_STA, "Failed to connect to AP: %s", esp_err_to_name(ret));
        current_status = WIFI_STATUS_FAILED;
    }

    return current_status;
}

#define DEFAULT_SCAN_LIST_SIZE 10

static void print_auth_mode(int authmode)
{
    switch (authmode) {
        case WIFI_AUTH_OPEN:                    ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_OPEN"); break;
        case WIFI_AUTH_OWE:                     ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_OWE"); break;
        case WIFI_AUTH_WEP:                     ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WEP"); break;
        case WIFI_AUTH_WPA_PSK:                 ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WPA_PSK"); break;
        case WIFI_AUTH_WPA2_PSK:                ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WPA2_PSK"); break;
        case WIFI_AUTH_WPA_WPA2_PSK:            ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK"); break;
        case WIFI_AUTH_ENTERPRISE:              ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_ENTERPRISE"); break;
        case WIFI_AUTH_WPA3_PSK:                ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WPA3_PSK"); break;
        case WIFI_AUTH_WPA2_WPA3_PSK:           ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK"); break;
        case WIFI_AUTH_WPA3_ENTERPRISE:         ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WPA3_ENTERPRISE"); break;
        case WIFI_AUTH_WPA2_WPA3_ENTERPRISE:    ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WPA2_WPA3_ENTERPRISE"); break;
        case WIFI_AUTH_WPA3_ENT_192:            ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_WPA3_ENT_192"); break;
        default:                                ESP_LOGI(TAG_AP, "Authmode \tWIFI_AUTH_UNKNOWN"); break;
    }
}

void wifi_scan(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_scan_start(NULL, true);

    ESP_LOGI(TAG_AP, "Max AP number ap_info can hold = %u", number);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_LOGI(TAG_AP, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);

    for (int i = 0; i < number; i++) {
        ESP_LOGI(TAG_AP, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG_AP, "RSSI \t\t%d", ap_info[i].rssi);

        print_auth_mode(ap_info[i].authmode);
        ESP_LOGI(TAG_AP, "Channel \t\t%d", ap_info[i].primary);
    }
}

