#include "common.h"
#include "esp_central.h"
#include "modlog/modlog.h"

#include "services/gap/ble_svc_gap.h"
#include "services/cts/ble_svc_cts.h"

static const char *TAG = "MOD_BLESCAN_HELPER";


static char *day_of_week[7] = {
    "Unknown" "Monday", "Tuesday", "Wednesday", "Thursday",
    "Friday", "Saturday", "Sunday"
};


int ble_cts_cent_on_read(uint16_t conn_handle, const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr, void *arg
) {
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


static int ble_htp_cent_on_subscribe_temp(uint16_t conn_handle,
                               const struct ble_gatt_error *error,
                               struct ble_gatt_attr *attr,
                               void *arg) {
    MODLOG_DFLT(INFO, "Subscribe to intermediate temperature char completed; status=%d "
                "conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);
    return 0;
}



/* 16 Bit Health Thermometer Service UUID */
#define BLE_SVC_HTP_UUID16                                   0x1809

/* 16 Bit Health Thermometer Service Characteristic UUIDs */
#define BLE_SVC_HTP_CHR_UUID16_TEMP_MEASUREMENT              0x2A1C
#define BLE_SVC_HTP_CHR_UUID16_TEMP_TYPE                     0x2A1D
#define BLE_SVC_HTP_CHR_UUID16_INTERMEDIATE_TEMP             0x2A1E
#define BLE_SVC_HTP_CHR_UUID16_MEASUREMENT_ITVL              0x2A21

/* 16 BIT CCCD UUID */
#define BLE_SVC_HTP_DSC_CLT_CFG_UUID16                       0x2902



static int ble_htp_cent_on_subscribe(uint16_t conn_handle, const struct ble_gatt_error *error,
                          struct ble_gatt_attr *attr, void *arg) {
    MODLOG_DFLT(INFO, "Subscribe to temperature measurement char completed; status=%d "
                "conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);
    const struct peer_dsc *dsc;
    uint8_t value[2];
    int rc;
    const struct peer *peer = peer_find(conn_handle);

    dsc = peer_dsc_find_uuid(peer,
                            BLE_UUID16_DECLARE(BLE_SVC_HTP_UUID16),
                            BLE_UUID16_DECLARE(BLE_SVC_HTP_CHR_UUID16_INTERMEDIATE_TEMP),
                            BLE_UUID16_DECLARE(BLE_SVC_HTP_DSC_CLT_CFG_UUID16));
    if (dsc == NULL) {
        MODLOG_DFLT(ERROR, "Error: Peer lacks a CCCD characteristic\n ");
        goto err;
    }

    value[0] = 1;
    value[1] = 0;

    rc = ble_gattc_write_flat(conn_handle, dsc->dsc.handle,
                            &value, sizeof value, ble_htp_cent_on_subscribe_temp, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to subscribe to characteristic; "
                    "rc=%d\n", rc);
        goto err;
    }

    return 0;

    err:
        /* Terminate the connection. */
        return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

static int ble_htp_cent_on_write(uint16_t conn_handle, const struct ble_gatt_error *error,
                      struct ble_gatt_attr *attr, void *arg) {
    MODLOG_DFLT(INFO, "Write to measurement interval char completed; status=%d "
                "conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);

    /* Subscribe to notifications for the temperature measurement characteristic.
     * A central enables notifications by writing two bytes (1, 0) to the
     * characteristic's client-characteristic-configuration-descriptor (CCCD).
     */
    const struct peer_dsc *dsc;
    uint8_t value[2];
    int rc;
    const struct peer *peer = peer_find(conn_handle);

    dsc = peer_dsc_find_uuid(peer, BLE_UUID16_DECLARE(BLE_SVC_HTP_UUID16),
                                    BLE_UUID16_DECLARE(BLE_SVC_HTP_CHR_UUID16_TEMP_MEASUREMENT),
                                    BLE_UUID16_DECLARE(BLE_SVC_HTP_DSC_CLT_CFG_UUID16));
    if (dsc == NULL) {
        MODLOG_DFLT(ERROR, "Error: Peer lacks a CCCD characteristic\n ");
        goto err;
    }

    value[0] = 2;
    value[1] = 0;

    rc = ble_gattc_write_flat(conn_handle, dsc->dsc.handle,
                                &value, sizeof value, ble_htp_cent_on_subscribe, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to subscribe to characteristic; "
                    "rc=%d\n", rc);
        goto err;
    }

    return 0;

    err:
        /* Terminate the connection. */
        return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}



int ble_htp_cent_on_read(uint16_t conn_handle, 
                        const struct ble_gatt_error *error,
                        struct ble_gatt_attr *attr, void *arg
) {
    MODLOG_DFLT(INFO, "Read temperature type char completed; status=%d conn_handle=%d",
                error->status, conn_handle);
    if (error->status == 0) {
        MODLOG_DFLT(INFO, " attr_handle=%d value=", attr->handle);
        print_mbuf(attr->om);
    }
    MODLOG_DFLT(INFO, "\n");

    /* Write two bytes (99, 100) to the measurement interval characteristic. */
    const struct peer_chr *chr;
    uint16_t value;
    int rc;
    const struct peer *peer = peer_find(conn_handle);

    chr = peer_chr_find_uuid(peer, BLE_UUID16_DECLARE(BLE_SVC_HTP_UUID16),
                                    BLE_UUID16_DECLARE(BLE_SVC_HTP_CHR_UUID16_MEASUREMENT_ITVL));
    if (chr == NULL) {
        MODLOG_DFLT(ERROR, "Error: Peer doesn't support the"
                    "measurement interval characteristic\n");
        goto err;
    }

    value = 2; /* Set measurement interval as 2 secs */
    rc = ble_gattc_write_flat(conn_handle, chr->chr.val_handle,
                                &value, sizeof value, ble_htp_cent_on_write, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "Error: Failed to write characteristic; rc=%d\n", rc);
        goto err;
    }

    return 0;

    err:
        /* Terminate the connection. */
        return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}
