#include "./common.h"

#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "MOD_BLEGatt";

// static uint8_t heart_rate;
// static const ble_uuid16_t heart_rate_svc_uuid = BLE_UUID16_INIT(0x180D);

// static uint8_t heart_rate_chr_val[2] = {0};
// static uint16_t heart_rate_chr_val_handle;
// static const ble_uuid16_t heart_rate_chr_uuid = BLE_UUID16_INIT(0x2A37);

// static uint16_t heart_rate_chr_conn_handle = 0;
// static bool heart_rate_chr_conn_handle_inited = false;
// static bool heart_rate_ind_status = false;

// /* Automation IO service */
// static const ble_uuid16_t auto_io_svc_uuid = BLE_UUID16_INIT(0x1815);
// static uint16_t led_chr_val_handle;
// static const ble_uuid128_t led_chr_uuid =
//     BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
//                     0x12, 0x12, 0x25, 0x15, 0x00, 0x00);

                    
// /* Note: Heart rate characteristic is read only */
// static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                                  struct ble_gatt_access_ctxt *ctxt, void *arg) {
//     int rc;
//     switch (ctxt->op) {
//         /* Read characteristic event */
//         case BLE_GATT_ACCESS_OP_READ_CHR:
//             //# Verify connection handle
//             if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//                 ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d", conn_handle, attr_handle);
//             } else {
//                 ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d", attr_handle);
//             }

//             //# Verify attribute handle
//             if (attr_handle == heart_rate_chr_val_handle) {
//                 /* Update access buffer value */
//                 heart_rate_chr_val[1] = heart_rate;
//                 rc = os_mbuf_append(ctxt->om, &heart_rate_chr_val, sizeof(heart_rate_chr_val));
//                 return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
//             }
//             goto error;

//         default:
//             goto error;
//     }

//     error:
//         ESP_LOGE(TAG, "unexpected access operation to heart rate characteristic, opcode: %d", ctxt->op);
//         return BLE_ATT_ERR_UNLIKELY;
// }

// /* Note: LED characteristic is write only */
// static int led_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
//     int rc;
//     switch (ctxt->op) {
//         /* Write characteristic event */
//         case BLE_GATT_ACCESS_OP_WRITE_CHR:
//             //# Verify connection handle
//             if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//                 ESP_LOGI(TAG, "characteristic write; conn_handle=%d attr_handle=%d", conn_handle, attr_handle);
//             } else {
//                 ESP_LOGI(TAG, "characteristic write by nimble stack; attr_handle=%d", attr_handle);
//             }

//             //# Verify attribute handle
//             if (attr_handle == led_chr_val_handle) {
//                 /* Verify access buffer length */
//                 if (ctxt->om->om_len == 1) {
//                     /* Turn the LED on or off according to the operation bit */
//                     if (ctxt->om->om_data[0]) {
//                         // led_on();
//                         ESP_LOGI(TAG, "led turned on!");
//                     } else {
//                         // led_off();
//                         ESP_LOGI(TAG, "led turned off!");
//                     }
//                 } else {
//                     goto error;
//                 }
//                 return rc;
//             }
//             goto error;

//         default:
//             goto error;
//     }

//     error:
//         ESP_LOGE(TAG, "unexpected access operation to led characteristic, opcode: %d", ctxt->op);
//         return BLE_ATT_ERR_UNLIKELY;
// }


// /* GATT services table */
// static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
//     {
//         //# Heart rate service
//         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//         .uuid = &heart_rate_svc_uuid.u,
//         .characteristics = (struct ble_gatt_chr_def[]){
//             {
//                 /* Heart rate characteristic */
//                 .uuid = &heart_rate_chr_uuid.u,
//                 .access_cb = heart_rate_chr_access,
//                 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
//                 .val_handle = &heart_rate_chr_val_handle
//             }, {
//                 0, /* No more characteristics in this service. */
//             }
//         }
//     }, {
//         //# Automation IO service
//         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//         .uuid = &auto_io_svc_uuid.u,
//         .characteristics = (struct ble_gatt_chr_def[]){
//             {
//                 /* LED characteristic */
//                 .uuid = &led_chr_uuid.u,
//                 .access_cb = led_chr_access,
//                 .flags = BLE_GATT_CHR_F_WRITE,
//                 .val_handle = &led_chr_val_handle
//             }, {
//                 0, /* No more characteristics in this service. */
//             }
//         },
//     }, {
//         0, /* No more services. */
//     },
// };

#include "services/cts/ble_svc_cts.h"

#include <time.h>
#include "sys/time.h"


static struct ble_svc_cts_local_time_info local_info = { .timezone = 0, .dst_offset = TIME_STANDARD };
static struct timeval last_updated;
uint8_t adjust_reason;
int fetch_current_time(struct ble_svc_cts_curr_time *ctime) {
    time_t now;
    struct tm timeinfo;
    struct timeval tv_now;
    /* time given by 'time()' api does not persist after reboots */
    time(&now);
    localtime_r(&now, &timeinfo);
    gettimeofday(&tv_now, NULL);
    if(ctime != NULL) {
        /* fill date_time */
        ctime->et_256.d_d_t.d_t.year = timeinfo.tm_year + 1900;
        ctime->et_256.d_d_t.d_t.month = timeinfo.tm_mon + 1;
        ctime->et_256.d_d_t.d_t.day = timeinfo.tm_mday;
        ctime->et_256.d_d_t.d_t.hours = timeinfo.tm_hour;
        ctime->et_256.d_d_t.d_t.minutes = timeinfo.tm_min;
        ctime->et_256.d_d_t.d_t.seconds = timeinfo.tm_sec;

        /* day of week */
        /* time gives day range of [0, 6], current_time_sevice
           has day range of [1,7] */
        ctime->et_256.d_d_t.day_of_week = timeinfo.tm_wday + 1;

        /* fractions_256 */
        ctime->et_256.fractions_256 = (((uint64_t)tv_now.tv_usec * 256L )/ 1000000L);

        ctime->adjust_reason = adjust_reason;
    }
    return 0;
}

int set_current_time(struct ble_svc_cts_curr_time ctime) {
    time_t now;
    struct tm timeinfo;
    struct timeval tv_now;
    /* fill date_time */
    timeinfo.tm_year= ctime.et_256.d_d_t.d_t.year - 1900 ;
    timeinfo.tm_mon = ctime.et_256.d_d_t.d_t.month - 1;
    timeinfo.tm_mday = ctime.et_256.d_d_t.d_t.day;
    timeinfo.tm_hour = ctime.et_256.d_d_t.d_t.hours;
    timeinfo.tm_min = ctime.et_256.d_d_t.d_t.minutes;
    timeinfo.tm_sec = ctime.et_256.d_d_t.d_t.seconds;
    timeinfo.tm_wday = ctime.et_256.d_d_t.day_of_week - 1;
    now = mktime(&timeinfo);
    tv_now.tv_sec = now;
    settimeofday(&tv_now, NULL);
    /* set the last updated */
    gettimeofday(&last_updated, NULL);
    adjust_reason = ctime.adjust_reason;
    return 0;
}

int fetch_local_time_info(struct ble_svc_cts_local_time_info *info) {

    if(info != NULL) {
        memcpy(info, &local_info, sizeof local_info);
    }
    return 0;
}

int set_local_time_info(struct ble_svc_cts_local_time_info info) {
    /* just store the dst offset and timezone locally
        as we don't have the access to time using ntp server */
    local_info.timezone = info.timezone;
    local_info.dst_offset = info.dst_offset;
    gettimeofday(&last_updated, NULL);
    return 0;
}

int fetch_reference_time_info(struct ble_svc_cts_reference_time_info *info) {
    struct timeval tv_now;
    uint64_t days_since_update;
    uint64_t hours_since_update;

    gettimeofday(&tv_now, NULL);
    /* subtract the time when the last time was updated */
    tv_now.tv_sec -= last_updated.tv_sec; /* ignore microseconds */
    info->time_source = TIME_SOURCE_MANUAL;
    info->time_accuracy = 0;
    days_since_update = (tv_now.tv_sec / 86400L);
    hours_since_update = (tv_now.tv_sec / 3600);
    info->days_since_update = days_since_update < 255 ? days_since_update : 255;

    if(days_since_update > 254) {
        info->hours_since_update = 255;
    }
    else {
        hours_since_update = (tv_now.tv_sec % 86400L) / 3600;
        info->hours_since_update = hours_since_update;
    }
    adjust_reason = (CHANGE_OF_DST_MASK | CHANGE_OF_TIME_ZONE_MASK);

    return 0;
}

int mod_BLEGatt_init() {
    int rc = 0;

    //# GATT service init
    ble_svc_gatt_init();

    // //# Update GATT services counter
    // int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    // if (rc != 0) {
    //     ESP_LOGE(TAG, "failed to set GATT service, error code: %d", rc);
    //     return rc;
    // } 

    // //# Add GATT services
    // rc = ble_gatts_add_svcs(gatt_svr_svcs);
    // if (rc != 0) {
    //     ESP_LOGE(TAG, "failed to add GATT service, error code: %d", rc);
    // }

    struct ble_svc_cts_cfg cfg;

    cfg.fetch_time_cb = fetch_current_time;
    cfg.local_time_info_cb = fetch_local_time_info;
    cfg.ref_time_info_cb = fetch_reference_time_info;
    cfg.set_time_cb = set_current_time;
    cfg.set_local_time_info_cb = set_local_time_info;
    ble_svc_cts_init(cfg);


    return rc;
}

#include "esp_random.h"

// void update_heart_rate(void) { heart_rate = 60 + (uint8_t)(esp_random() % 21); }

// void send_heart_rate_indication(void) {
//     if (heart_rate_ind_status && heart_rate_chr_conn_handle_inited) {
//         ble_gatts_indicate(heart_rate_chr_conn_handle, heart_rate_chr_val_handle);
//         ESP_LOGI(TAG, "heart rate indication sent!");
//     }
// }

// void mod_heart_rate_task(void *param) {
//     ESP_LOGI(TAG, "heart rate task has been started!");

//     while (1) {
//         /* Update heart rate value every 2 second */
//         update_heart_rate();
//         ESP_LOGI(TAG, "heart rate updated to %d", heart_rate);
//         send_heart_rate_indication();
//         vTaskDelay(2000 / portTICK_PERIOD_MS);      // delay
//     }

//     vTaskDelete(NULL);      /* Clean up at exit */
// }