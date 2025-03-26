
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int ble_cts_cent_on_read(
    uint16_t conn_handle, 
    const struct ble_gatt_error *error,
    struct ble_gatt_attr *attr, void *arg
);


int ble_htp_cent_on_read(
    uint16_t conn_handle, 
    const struct ble_gatt_error *error,
    struct ble_gatt_attr *attr, void *arg
);