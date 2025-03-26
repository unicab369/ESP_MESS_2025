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