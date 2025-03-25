#include "mod_BLEGap.h"

#include "services/gap/ble_svc_gap.h"


static const char *TAG = "MOD_BLEGat";
static const char* DEVICE_NAME = "NimBLE_Service";


static uint8_t own_addr_type;
static uint8_t addr_val[6] = {0};
static uint8_t esp_uri[] = {BLE_GAP_URI_PREFIX_HTTPS, '/', '/', 'e', 's', 'p', 'r', 'e', 's', 's', 'i', 'f', '.', 'c', 'o', 'm'};

static int start_advertising();
static bool beacon_only = false;

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                        event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                        event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    // if (event->subscribe.attr_handle == heart_rate_chr_val_handle) {
    //     /* Update heart rate subscription status */
    //     heart_rate_chr_conn_handle = event->subscribe.conn_handle;
    //     heart_rate_chr_conn_handle_inited = true;
    //     heart_rate_ind_status = event->subscribe.cur_indicate;
    // }
}

inline static void format_addr(char *addr_str, uint8_t addr[]) {
    sprintf(addr_str, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);
}

static void print_conn_desc(struct ble_gap_conn_desc *desc) {
    /* Local variables */
    char addr_str[18] = {0};

    /* Connection handle */
    ESP_LOGI(TAG, "connection handle: %d", desc->conn_handle);

    /* Local ID address */
    format_addr(addr_str, desc->our_id_addr.val);
    ESP_LOGI(TAG, "device id address: type=%d, value=%s",
                desc->our_id_addr.type, addr_str);

    /* Peer ID address */
    format_addr(addr_str, desc->peer_id_addr.val);
    ESP_LOGI(TAG, "peer id address: type=%d, value=%s", desc->peer_id_addr.type,
                addr_str);

    /* Connection info */
    ESP_LOGI(TAG,
                "conn_itvl=%d, conn_latency=%d, supervision_timeout=%d, "
                "encrypted=%d, authenticated=%d, bonded=%d\n",
                desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
                desc->sec_state.encrypted, desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

static int gap_event_handler(struct ble_gap_event *event, void *arg) {
    int rc = 0;
    struct ble_gap_conn_desc desc;

    switch (event->type) {

    /* Connect event */
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "connection %s; status=%d",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);

        /* Connection succeeded */
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            if (rc != 0) {
                ESP_LOGE(TAG, "failed to find connection by handle, error code: %d", rc);
                return rc;
            }

            /* Print connection descriptor and turn on the LED */
            print_conn_desc(&desc);

            /* Try to update connection parameters */
            struct ble_gap_upd_params params = {.itvl_min = desc.conn_itvl,
                                            .itvl_max = desc.conn_itvl,
                                            .latency = 3,
                                            .supervision_timeout = desc.supervision_timeout};
            rc = ble_gap_update_params(event->connect.conn_handle, &params);
            if (rc != 0) {
                ESP_LOGE(TAG, "failed to update connection parameters, error code: %d", rc);
                return rc;
            }
        }
        /* Connection failed, restart advertising */
        else {
            start_advertising();
        }
        return rc;

    /* Disconnect event */
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnected from peer; reason=%d", event->disconnect.reason);

        /* Restart advertising */
        start_advertising();
        return rc;

    /* Connection parameters update event */
    case BLE_GAP_EVENT_CONN_UPDATE:
        ESP_LOGI(TAG, "connection updated; status=%d", event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        if (rc != 0) {
            ESP_LOGE(TAG, "failed to find connection by handle, error code: %d", rc);
            return rc;
        }
        print_conn_desc(&desc);
        return rc;

    /* Advertising complete event */
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "advertise complete; reason=%d", event->adv_complete.reason);
        start_advertising();
        return rc;

    /* Notification sent event */
    case BLE_GAP_EVENT_NOTIFY_TX:
        if ((event->notify_tx.status != 0) && (event->notify_tx.status != BLE_HS_EDONE)) {
            ESP_LOGI(TAG, "notify event; conn_handle=%d attr_handle=%d "
                            "status=%d is_indication=%d",
                            event->notify_tx.conn_handle, event->notify_tx.attr_handle,
                            event->notify_tx.status, event->notify_tx.indication);
        }
        return rc;

    /* Subscribe event */
    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d",
                    event->subscribe.conn_handle, event->subscribe.attr_handle,
                    event->subscribe.reason, event->subscribe.prev_notify,
                    event->subscribe.cur_notify, event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);

        /* GATT subscribe event callback */
        gatt_svr_subscribe_cb(event);
        return rc;

    /* MTU update event */
    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
                event->mtu.conn_handle, event->mtu.channel_id,
                event->mtu.value);
        return rc;
    }

    return rc;
}


//! Current Time Service: extended adv
#if CONFIG_EXAMPLE_EXTENDED_ADV
    static void ext_ble_cts_prph_advertise(void) {
        struct ble_gap_ext_adv_params params;
        struct os_mbuf *data;
        uint8_t instance = 0;
        int rc;

        /* First check if any instance is already active */
        if (ble_gap_ext_adv_active(instance)) {
            return;
        }

        /* use defaults for non-set params */
        memset (&params, 0, sizeof(params));

        /* enable connectable advertising */
        params.connectable = 1;

        /* advertise using random addr */
        params.own_addr_type = BLE_OWN_ADDR_PUBLIC;

        params.primary_phy = BLE_HCI_LE_PHY_1M;
        params.secondary_phy = BLE_HCI_LE_PHY_2M;
        params.sid = 1;

        params.itvl_min = BLE_GAP_ADV_FAST_INTERVAL1_MIN;
        params.itvl_max = BLE_GAP_ADV_FAST_INTERVAL1_MIN;

        /* configure instance 0 */
        rc = ble_gap_ext_adv_configure(instance, &params, NULL, ble_cts_prph_gap_event, NULL);
        assert (rc == 0);

        /* in this case only scan response is allowed */

        /* get mbuf for scan rsp data */
        data = os_msys_get_pkthdr(sizeof(ext_adv_pattern_1), 0);
        assert(data);

        /* fill mbuf with scan rsp data */
        rc = os_mbuf_append(data, ext_adv_pattern_1, sizeof(ext_adv_pattern_1));
        assert(rc == 0);

        rc = ble_gap_ext_adv_set_data(instance, data);
        assert (rc == 0);

        /* start advertising */
        rc = ble_gap_ext_adv_start(instance, 0, 0);
        assert (rc == 0);
    }
#else
    static void ble_cts_prph_advertise(void) {

    }
#endif



static int start_advertising() {
    //# Set device name
    const char *name = ble_svc_gap_device_name();
    struct ble_hs_adv_fields adv_fields = {0};
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

    /* Set device tx power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    /* Set device appearance */
    adv_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
    adv_fields.appearance_is_present = 1;

    // /* Set device LE role */
    // adv_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
    // adv_fields.le_role_is_present = 1;

    //! Current Time Service
    /* 16 Bit Current Time Service UUID */
    #define BLE_SVC_CTS_UUID16                      0x1805

    adv_fields.uuids16 = (ble_uuid16_t[]) {
        BLE_UUID16_INIT(BLE_SVC_CTS_UUID16)
    };
    adv_fields.num_uuids16 = 1;
    adv_fields.uuids16_is_complete = 1;

    //# Set dvertisement fields
    int rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return rc;
    }

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! DIFF
    //# Set device address */
    struct ble_hs_adv_fields rsp_fields = {0};
    rsp_fields.device_addr = addr_val;
    rsp_fields.device_addr_type = own_addr_type;
    rsp_fields.device_addr_is_present = 1;

    /* Set URI */
    rsp_fields.uri = esp_uri;
    rsp_fields.uri_len = sizeof(esp_uri);

    //! NOTE: Beacon doesn't set response field adv interval
    if (!beacon_only) {
        /* Set advertising interval */
        rsp_fields.adv_itvl = BLE_GAP_ADV_ITVL_MS(500);
        rsp_fields.adv_itvl_is_present = 1;
    }

    //# Set scan response fields
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set scan response data, error code: %d", rc);
        return rc;
    }

    //# Set non-connetable and general discoverable mode to be a beacon
    if (beacon_only) {
        printf("BLE Mode: Beacon\n");
        struct ble_gap_adv_params adv_params = {0};
        adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
        adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

        //! Start advertising
        return ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, 
                                &adv_params, NULL, NULL);
    } else {
        printf("BLE Mode: Connection\n");
        struct ble_gap_adv_params adv_params = {0};
        adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
        adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

        /* Set advertising interval */
        adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(500);
        adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(510);

        //#!Start advertising
        return ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                                &adv_params, gap_event_handler, NULL);
    }
}

void on_stack_sync() {
    int rc = 0;
    char addr_str[18] = {0};

    //# Make sure we have proper BT identity address set
    rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "device does not have any available bt address!");
        return;
    }

    //# Figure out BT address to use while advertising
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to infer address type, error code: %d", rc);
        return;
    }

    //# Copy device address to addr_val
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to copy device address, error code: %d", rc);
        return;
    }
    format_addr(addr_str, addr_val);
    ESP_LOGI(TAG, "device address: %s", addr_str);

    //! Start advertising
    rc = start_advertising();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "advertising started!");
}


int mod_BLEGap_init(bool beacon) {
    beacon_only = beacon;

    //# GAP service init
    ble_svc_gap_init();

    //# Set GAP device name
    int rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device name to %s, error code: %d", DEVICE_NAME, rc);
        return rc;
    }

    //# Set GAP device appearance
    rc = ble_svc_gap_device_appearance_set(BLE_GAP_APPEARANCE_GENERIC_TAG);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device appearance, error code: %d", rc);
        return rc;
    }

    return rc;
}
