#include "./common.h"

#include "modlog/modlog.h"

//! dependency
#include "esp_central.h"

#include "services/gap/ble_svc_gap.h"
#include "services/cts/ble_svc_cts.h"

static const char *TAG = "NimBLE_CTS_CENT";

static void start_scan(void);
static int handle_gap_event(struct ble_gap_event *event, void *arg);

static int ble_cts_cent_gap_event(struct ble_gap_event *event, void *arg);

static uint8_t peer_addr[6];

static char *day_of_week[7] = {
    "Unknown" "Monday", "Tuesday", "Wednesday", "Thursday",
    "Friday", "Saturday", "Sunday"
};


static int ble_cts_cent_on_read(uint16_t conn_handle, const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr, void *arg) {
    struct ble_svc_cts_curr_time ctime; /* store the read time */
    MODLOG_DFLT(INFO, "Read Current time complete; status=%d conn_handle=%d\n",
                error->status, conn_handle);
    if (error->status == 0) {
        MODLOG_DFLT(INFO, " attr_handle=%d value=\n", attr->handle);
        print_mbuf(attr->om);
    } else {
        goto err;
    }
    MODLOG_DFLT(INFO, "\n");
    ble_hs_mbuf_to_flat(attr->om, &ctime, sizeof(ctime), NULL);

    //# Print time
    ESP_LOGI(TAG, "Date : %d/%d/%d %s", ctime.et_256.d_d_t.d_t.day,
        ctime.et_256.d_d_t.d_t.month, ctime.et_256.d_d_t.d_t.year,
        day_of_week[ctime.et_256.d_d_t.day_of_week]);
    ESP_LOGI(TAG, "hours : %d minutes : %d ", ctime.et_256.d_d_t.d_t.hours, ctime.et_256.d_d_t.d_t.minutes);
    ESP_LOGI(TAG, "seconds : %d\n", ctime.et_256.d_d_t.d_t.seconds);
    ESP_LOGI(TAG, "fractions : %d\n", ctime.et_256.fractions_256);
    return 0;

    err:
        /* Terminate the connection. */
        return ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}


// //! SAME (adjust)
static void on_connect_handler(const struct peer *peer, int status, void *arg) {
    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        MODLOG_DFLT(ERROR, "Error: Service discovery failed; status=%d "
                    "conn_handle=%d\n", status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    /* Service discovery has completed successfully.  Now we have a complete
     list of services, characteristics, and descriptors that the peer supports. */
    MODLOG_DFLT(INFO, "Service discovery complete; status=%d conn_handle=%d\n", status, peer->conn_handle);

    const struct peer_chr *chr = peer_chr_find_uuid(peer, BLE_UUID16_DECLARE(BLE_SVC_CTS_UUID16),
                                    BLE_UUID16_DECLARE(BLE_SVC_CTS_CHR_UUID16_CURRENT_TIME));
    if (chr == NULL) {
        MODLOG_DFLT(ERROR, "Error: Peer doesn't support the CTS characteristic\n");
        goto err;
    }
    int rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle, ble_cts_cent_on_read, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to read characteristic; rc=%d\n", rc);
        goto err;
    }

    err:
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

//! SAME (adjust)
/* Indicates whether we should try to connect to the sender of the specified
    advertisement.  The function returns a positive result if the device
    advertises connectability and support for the matching Service. */
#if CONFIG_EXAMPLE_EXTENDED_ADV
    static int should_connect(const struct ble_gap_ext_disc_desc *disc) {
        int offset = 0;
        int ad_struct_len = 0;

        if (disc->legacy_event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
                disc->legacy_event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
            return 0;
        }
        if (strlen(CONFIG_EXAMPLE_PEER_ADDR) && (strncmp(CONFIG_EXAMPLE_PEER_ADDR, "ADDR_ANY", strlen("ADDR_ANY")) != 0)) {
            ESP_LOGI(tag, "Peer address from menuconfig: %s", CONFIG_EXAMPLE_PEER_ADDR);
            /* Convert string to address */
            sscanf(CONFIG_EXAMPLE_PEER_ADDR, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &peer_addr[5], &peer_addr[4], &peer_addr[3],
                &peer_addr[2], &peer_addr[1], &peer_addr[0]);
            if (memcmp(peer_addr, disc->addr.val, sizeof(disc->addr.val)) != 0) return 0;
        }

        /* The device has to advertise support for the CTS service (0x1805).*/
        do {
            ad_struct_len = disc->data[offset];
            if (!ad_struct_len) break;

            //! Search if CTS UUID is advertised
            if (disc->data[offset + 1] == 0x03) {
                int temp = 2;
                while(temp < disc->data[offset + 1]) {
                    if(disc->data[offset + temp] == 0x05 && disc->data[offset + temp + 1] == 0x18) {
                        return 1;
                    }
                    temp += 2;
                }
            }

            // //! Search if HTP UUID is advertised
            // if ((disc->data[offset] == 0x03 && disc->data[offset + 1] == 0x03)
            //         && (disc->data[offset + 2] == 0x18 && disc->data[offset + 3] == 0x09)) {
            //     return 1;
            // }
            offset += ad_struct_len + 1;
        } while ( offset < disc->length_data );

        return 0;
    }
#else
    static int should_connect(const struct ble_gap_disc_desc *disc) {
        struct ble_hs_adv_fields fields;
        int rc;
        int i;

        /* The device has to be advertising connectability. */
        if (disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND &&
                disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
            return 0;
        }

        rc = ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data);
        if (rc != 0) return 0;

        // if (strlen(CONFIG_EXAMPLE_PEER_ADDR) && (strncmp(CONFIG_EXAMPLE_PEER_ADDR, "ADDR_ANY", strlen("ADDR_ANY")) != 0)) {
        //     ESP_LOGI(tag, "Peer address from menuconfig: %s", CONFIG_EXAMPLE_PEER_ADDR);
        //     /* Convert string to address */
        //     sscanf(CONFIG_EXAMPLE_PEER_ADDR, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        //         &peer_addr[5], &peer_addr[4], &peer_addr[3],
        //         &peer_addr[2], &peer_addr[1], &peer_addr[0]);
        //     if (memcmp(peer_addr, disc->addr.val, sizeof(disc->addr.val)) != 0) {
        //         return 0;
        //     }
        // }

        // The device has to advertise support for the Current Time service (0x1805).
        for (i = 0; i < fields.num_uuids16; i++) {
            uint16_t service_id = ble_uuid_u16(&fields.uuids16[i].u);
            printf("check service_Id: %u, comp_id: %u\n", service_id, BLE_SVC_CTS_UUID16);
            if (service_id == BLE_SVC_CTS_UUID16) return 1;
        }

        return 0;
    }
#endif


/* Connects to the sender of the specified advertisement of it looks
 interesting.  A device is "interesting" if it advertises compatible service. */
static void connect_if_interesting(void *disc) {
    uint8_t own_addr_type;
    int rc;
    ble_addr_t *addr;

    /* Don't do anything if we don't care about this advertiser. */
    #if CONFIG_EXAMPLE_EXTENDED_ADV
        if (!should_connect((struct ble_gap_ext_disc_desc *)disc)) return;
    #else
        if (!should_connect((struct ble_gap_disc_desc *)disc)) return;
    #endif

    #if !(MYNEWT_VAL(BLE_HOST_ALLOW_CONNECT_WITH_SCAN))
        /* Scanning must be stopped before a connection can be initiated. */
        rc = ble_gap_disc_cancel();
        if (rc != 0) {
            MODLOG_DFLT(DEBUG, "Failed to cancel scan; rc=%d\n", rc);
            return;
        }
    #endif

    /* Figure out address to use for connect (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Try to connect the the advertiser.  Allow 30 seconds (30000 ms) for timeout */
    #if CONFIG_EXAMPLE_EXTENDED_ADV
        addr = &((struct ble_gap_ext_disc_desc *)disc)->addr;
    #else
        addr = &((struct ble_gap_disc_desc *)disc)->addr;
    #endif

    printf("connecting to device\n");

    rc = ble_gap_connect(own_addr_type, addr, 30000, NULL, handle_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to connect to device; addr_type=%d "
                    "addr=%s; rc=%d\n", addr->type, addr_str(addr->val), rc);
        return;
    }
}

static int handle_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if (rc != 0) return 0;

        print_adv_fields(&fields);
        connect_if_interesting(&event->disc);
        return 0;

    case BLE_GAP_EVENT_LINK_ESTAB:
        if (event->connect.status == 0) {
            MODLOG_DFLT(INFO, "Connection established ");

            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            print_conn_desc(&desc);
            MODLOG_DFLT(INFO, "\n");

            /* Remember peer. */
            rc = peer_add(event->connect.conn_handle);
            if (rc != 0) {
                MODLOG_DFLT(ERROR, "Failed to add peer; rc=%d\n", rc);
                return 0;
            }

    #if CONFIG_EXAMPLE_ENCRYPTION
            /** Initiate security - It will perform
             * Pairing (Exchange keys)
             * Bonding (Store keys)
             * Encryption (Enable encryption)
             * Will invoke event BLE_GAP_EVENT_ENC_CHANGE
             **/
            rc = ble_gap_security_initiate(event->connect.conn_handle);
            if (rc != 0) {
                MODLOG_DFLT(INFO, "Security could not be initiated, rc = %d\n", rc);
                return ble_gap_terminate(event->connect.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            } else {
                MODLOG_DFLT(INFO, "Connection secured\n");
            }
    #else
            /* Perform service discovery */
            rc = peer_disc_all(event->connect.conn_handle, on_connect_handler, NULL);
            if (rc != 0) {
                MODLOG_DFLT(ERROR, "Failed to discover services; rc=%d\n", rc);
                return 0;
            }
    #endif
        } else {
            /* Connection attempt failed; resume scanning. */
            MODLOG_DFLT(ERROR, "Error: Connection failed; status=%d\n", event->connect.status);
            start_scan();
        }

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
        print_conn_desc(&event->disconnect.conn);
        MODLOG_DFLT(INFO, "\n");

        /* Forget about peer. */
        peer_delete(event->disconnect.conn.conn_handle);
        start_scan();
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        MODLOG_DFLT(INFO, "discovery complete; reason=%d\n", event->disc_complete.reason);
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        MODLOG_DFLT(INFO, "encryption change event; status=%d ", event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        print_conn_desc(&desc);

    #if CONFIG_EXAMPLE_ENCRYPTION
        /*** Go for service discovery after encryption has been successfully enabled ***/
        rc = peer_disc_all(event->connect.conn_handle, on_connect_handler, NULL);
        if (rc != 0) {
            MODLOG_DFLT(ERROR, "Failed to discover services; rc=%d\n", rc);
            return 0;
        }
    #endif
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        /* Peer sent us a notification or indication. */
        MODLOG_DFLT(INFO, "received %s; conn_handle=%d attr_handle=%d attr_len=%d\n",
                    event->notify_rx.indication ? "indication" : "notification",
                    event->notify_rx.conn_handle,
                    event->notify_rx.attr_handle,
                    OS_MBUF_PKTLEN(event->notify_rx.om));

        /* Attribute data is contained in event->notify_rx.om. Use
         * `os_mbuf_copydata` to copy the data received in notification mbuf */
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle, event->mtu.channel_id, event->mtu.value);
        return 0;

#if CONFIG_EXAMPLE_EXTENDED_ADV
    case BLE_GAP_EVENT_EXT_DISC:
        /* An advertisement report was received during GAP discovery. */
        ext_print_adv_report(&event->disc);
        connect_if_interesting(&event->disc);
        return 0;
#endif

    default:
        return 0;
    }
}


static void start_scan(void) {
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params;
    int rc;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Tell the controller to filter duplicates; we don't want to process
     * repeated advertisements from the same device. */
    disc_params.filter_duplicates = 1;

    /* Perform a passive scan.  I.e., don't send follow-up scan requests to
     * each advertiser. */
    disc_params.passive = 1;
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    ESP_LOGW(TAG, "Start Scanning ");
    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params, handle_gap_event, NULL);

    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error initiating GAP discovery procedure; rc=%d\n", rc);
    }
}

static void on_scan_sync() {
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    //# Begin scanning for a peripheral to connect to
    start_scan();
}


static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

void mod_nimbleBLE_task(void *param) {
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();
    vTaskDelete(NULL);      /* Clean up at exit */
}

void mod_BLEScan_setup() {
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d ", ret);
        return;
    }

    /* Configure the host. */
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_scan_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Initialize data structures to track connected peers. */
    int rc = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 64, 64, 64);
    assert(rc == 0);

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set("nimble-cts-cent");
    assert(rc == 0);

    //# start task
    xTaskCreate(mod_nimbleBLE_task, "NimBLE Host", 4*1024, NULL, 5, NULL);
}