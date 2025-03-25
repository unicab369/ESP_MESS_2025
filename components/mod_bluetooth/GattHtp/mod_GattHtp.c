#include "./common.h"

#include "nimble/ble.h"

#include "services/gatt/ble_svc_gatt.h"
#include "services/htp/ble_svc_htp.h"


static const char *TAG = "MOD_GATT_HTP";

/* 16 Bit Device Information Service Characteristic UUIDs */
#define GATT_DIS_DEVICE_INFO_UUID                         0x180A
#define GATT_DIS_CHR_UUID16_SYS_ID                        0x2A23
#define GATT_DIS_CHR_UUID16_MODEL_NO                      0x2A24
#define GATT_DIS_CHR_UUID16_MFC_NAME                      0x2A29


static const char *manuf_name = "ESP32 devkitC";
static const char *model_num = "HTP Sensor demo";
static const char *system_id = "HTP1";

static int
gatt_svr_chr_access_device_info(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg) {
    uint16_t uuid = ble_uuid_u16(ctxt->chr->uuid);
    int rc;

    if (uuid == GATT_DIS_CHR_UUID16_MODEL_NO) {
        rc = os_mbuf_append(ctxt->om, model_num, strlen(model_num));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_DIS_CHR_UUID16_MFC_NAME) {
        rc = os_mbuf_append(ctxt->om, manuf_name, strlen(manuf_name));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (uuid == GATT_DIS_CHR_UUID16_SYS_ID) {
        rc = os_mbuf_append(ctxt->om, system_id, strlen(system_id));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}


static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(GATT_DIS_DEVICE_INFO_UUID),
        .characteristics = (struct ble_gatt_chr_def[]) { 
            {
                /* Characteristic: * Manufacturer name */
                .uuid = BLE_UUID16_DECLARE(GATT_DIS_CHR_UUID16_MFC_NAME),
                .access_cb = gatt_svr_chr_access_device_info,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                /* Characteristic: Model number string */
                .uuid = BLE_UUID16_DECLARE(GATT_DIS_CHR_UUID16_MODEL_NO),
                .access_cb = gatt_svr_chr_access_device_info,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                /* Characteristic: System ID */
                .uuid = BLE_UUID16_DECLARE(GATT_DIS_CHR_UUID16_SYS_ID),
                .access_cb = gatt_svr_chr_access_device_info,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                0, /* No more characteristics in this service */
            },
        }
    }, {
        0, /* No more services */
    },
};

int mod_GattHtp_init() {
    //# GATT service init
    ble_svc_gatt_init();

    //# Update GATT services counter
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set GATT service, error code: %d", rc);
        return rc;
    } 

    //# Add GATT services
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to add GATT service, error code: %d", rc);
    }

    return rc;
}