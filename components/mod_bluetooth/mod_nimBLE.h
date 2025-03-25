

#define FLAG_BLE_ENABLED 1

void mod_nimbleBLE_setup(bool beacon_only);

void mod_nimbleBLE_task(void *param);
void mod_heart_rate_task(void *param);