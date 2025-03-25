/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include "common.h"

#include "mod_nimBLE.h"

#include "BLEGap/mod_BLEGap.h"
#include "BLEGatt/mod_BLEGatt.h"

static const char *TAG = "MOD_NIMBLE";

static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

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

    //! Set host callbacks
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    //# GAP service init
    int rc = mod_BLEGap_init(beacon);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", rc);
        return;
    }

    if (!beacon) {
        //! Current Time Service: Secure connection
        ble_hs_cfg.sm_bonding = 1;
        ble_hs_cfg.sm_sc = 1;
        ble_hs_cfg.sm_mitm = 1;
        ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
        ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

        //# GATT service init
        rc = mod_BLEGatt_init();
        if (rc != 0) {
            ESP_LOGE(TAG, "failed to initialize GATT server, error code: %d", rc);
            return;
        }
    }

    /* Store host configuration */
    // ble_store_config_init();
}


void mod_nimbleBLE_task(void *param) {
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();
    vTaskDelete(NULL);      /* Clean up at exit */
}