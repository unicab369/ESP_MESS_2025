/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "common.h"

#include "mod_nimBLE.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "MOD_NIMBLE_SERVICE";
static const char* DEVICE_NAME = "NimBLE_Service";

/* Private variables */
static uint8_t own_addr_type;
static uint8_t addr_val[6] = {0};
static uint8_t esp_uri[] = {BLE_GAP_URI_PREFIX_HTTPS, '/', '/', 'e', 's', 'p', 'r', 'e', 's', 's', 'i', 'f', '.', 'c', 'o', 'm'};
static bool beacon_only = true;


static const ble_uuid16_t heart_rate_svc_uuid = BLE_UUID16_INIT(0x180D);

static uint8_t heart_rate_chr_val[2] = {0};
static uint16_t heart_rate_chr_val_handle;
static const ble_uuid16_t heart_rate_chr_uuid = BLE_UUID16_INIT(0x2A37);

static uint16_t heart_rate_chr_conn_handle = 0;
static bool heart_rate_chr_conn_handle_inited = false;
static bool heart_rate_ind_status = false;

/* Automation IO service */
static const ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x1815);
static uint16_t led_chr_val_handle;
static const ble_uuid128_t led_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                    0x12, 0x12, 0x25, 0x15, 0x00, 0x00);



static int start_service_advertising();

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
    if (event->subscribe.attr_handle == heart_rate_chr_val_handle) {
        /* Update heart rate subscription status */
        heart_rate_chr_conn_handle = event->subscribe.conn_handle;
        heart_rate_chr_conn_handle_inited = true;
        heart_rate_ind_status = event->subscribe.cur_indicate;
    }
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
            start_service_advertising();
        }
        return rc;

    /* Disconnect event */
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnected from peer; reason=%d", event->disconnect.reason);

        /* Restart advertising */
        start_service_advertising();
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
        start_service_advertising();
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


static int start_service_advertising() {
    /* Set advertising flags */
    struct ble_hs_adv_fields adv_fields = {0};
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    //# Set device name
    const char *name = ble_svc_gap_device_name();
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

    /* Set device tx power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    /* Set device appearance */
    adv_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
    adv_fields.appearance_is_present = 1;

    /* Set device LE role */
    adv_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
    adv_fields.le_role_is_present = 1;

    //# Set dvertisement fields
    int rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return rc;
    }

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


static void on_sync_change() {
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
    rc = start_service_advertising();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start advertising, error code: %d", rc);
        return;
    }
    ESP_LOGI(TAG, "advertising started!");
}


static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}


static uint8_t heart_rate;

/* Note: Heart rate characteristic is read only */
static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;
    switch (ctxt->op) {
        /* Read characteristic event */
        case BLE_GATT_ACCESS_OP_READ_CHR:
            //# Verify connection handle
            if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d", conn_handle, attr_handle);
            } else {
                ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d", attr_handle);
            }

            //# Verify attribute handle
            if (attr_handle == heart_rate_chr_val_handle) {
                /* Update access buffer value */
                heart_rate_chr_val[1] = heart_rate;
                rc = os_mbuf_append(ctxt->om, &heart_rate_chr_val, sizeof(heart_rate_chr_val));
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            }
            goto error;

        default:
            goto error;
    }

    error:
        ESP_LOGE(TAG, "unexpected access operation to heart rate characteristic, opcode: %d", ctxt->op);
        return BLE_ATT_ERR_UNLIKELY;
}

/* Note: LED characteristic is write only */
static int led_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int rc;
    switch (ctxt->op) {
        /* Write characteristic event */
        case BLE_GATT_ACCESS_OP_WRITE_CHR:
            //# Verify connection handle
            if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d", conn_handle, attr_handle);
            } else {
                ESP_LOGI(TAG, "characteristic write by nimble stack; attr_handle=%d", attr_handle);
            }

            //# Verify attribute handle
            if (attr_handle == led_chr_val_handle) {
                /* Verify access buffer length */
                if (ctxt->om->om_len == 1) {
                    /* Turn the LED on or off according to the operation bit */
                    if (ctxt->om->om_data[0]) {
                        // led_on();
                        ESP_LOGI(TAG, "led turned on!");
                    } else {
                        // led_off();
                        ESP_LOGI(TAG, "led turned off!");
                    }
                } else {
                    goto error;
                }
                return rc;
            }
            goto error;

        default:
            goto error;
    }

    error:
        ESP_LOGE(TAG, "unexpected access operation to led characteristic, opcode: %d", ctxt->op);
        return BLE_ATT_ERR_UNLIKELY;
}





/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        //# Heart rate service
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &heart_rate_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                /* Heart rate characteristic */
                .uuid = &heart_rate_chr_uuid.u,
                .access_cb = heart_rate_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
                .val_handle = &heart_rate_chr_val_handle
            }, {
                0, /* No more characteristics in this service. */
            }
        }
    }, {
        //# Automation IO service
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &auto_io_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                /* LED characteristic */
                .uuid = &led_chr_uuid.u,
                .access_cb = led_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle = &led_chr_val_handle
            }, {
                0, /* No more characteristics in this service. */
            }
        },
    }, {
        0, /* No more services. */
    },
};


void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {
        /* Service register event */
        case BLE_GATT_REGISTER_OP_SVC:
            ESP_LOGD(TAG, "registered service %s with handle=%d",
                            ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                            ctxt->svc.handle); break;

        /* Characteristic register event */
        case BLE_GATT_REGISTER_OP_CHR:
            ESP_LOGD(TAG, "registering characteristic %s with "
                            "def_handle=%d val_handle=%d",
                            ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                            ctxt->chr.def_handle, ctxt->chr.val_handle); break;

        /* Descriptor register event */
        case BLE_GATT_REGISTER_OP_DSC:
            ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                            ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                            ctxt->dsc.handle); break;
        default:
            assert(0); break;
    }
}

void mod_nimbleBLE_setup(bool beacon) {
    //# NimBLE stack initialization
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ", ret);
        return;
    }

    //# Set GAP device name
    int rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device name to %s, error code: %d", DEVICE_NAME, rc);
        return;
    }

    //# Set GAP device appearance
    rc = ble_svc_gap_device_appearance_set(BLE_GAP_APPEARANCE_GENERIC_TAG);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device appearance, error code: %d", rc);
        return;
    }

    beacon_only = beacon;
    if (beacon_only) {
        
    } else {
        //# GAP service init
        ble_svc_gap_init();
        
        //# GATT service init
        ble_svc_gatt_init();

        //# Update GATT services counter
        int rc = ble_gatts_count_cfg(gatt_svr_svcs);
        if (rc != 0) {
            ESP_LOGE(TAG, "failed to set GATT service, error code: %d", rc);
            return;
        } 
    
        //# Add GATT services
        rc = ble_gatts_add_svcs(gatt_svr_svcs);
        if (rc != 0) {
            ESP_LOGE(TAG, "failed to add GATT service, error code: %d", rc);
            return;
        } 
    
        ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    }

    /* Set host callbacks */
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_sync_change;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

        
    /* Store host configuration */
    // ble_store_config_init();
}


void mod_nimbleBLE_task(void *param) {
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();
    vTaskDelete(NULL);      /* Clean up at exit */
}

#include "esp_random.h"

void update_heart_rate(void) { heart_rate = 60 + (uint8_t)(esp_random() % 21); }

void send_heart_rate_indication(void) {
    if (heart_rate_ind_status && heart_rate_chr_conn_handle_inited) {
        ble_gatts_indicate(heart_rate_chr_conn_handle, heart_rate_chr_val_handle);
        ESP_LOGI(TAG, "heart rate indication sent!");
    }
}

void mod_heart_rate_task(void *param) {
    ESP_LOGI(TAG, "heart rate task has been started!");

    while (1) {
        /* Update heart rate value every 2 second */
        update_heart_rate();
        ESP_LOGI(TAG, "heart rate updated to %d", heart_rate);
        send_heart_rate_indication();
        vTaskDelay(2000 / portTICK_PERIOD_MS);      // delay
    }

    vTaskDelete(NULL);      /* Clean up at exit */
}