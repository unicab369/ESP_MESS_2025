#include <stddef.h>
#include <string.h>
#include "nvs_flash.h"
#include <errno.h>

#include "nvs.h"
#include "esp_log.h"
#include "esp_err.h"


void mod_nvs_setup(void);
int mod_nvs_set_namespace(const char* target);

esp_err_t mod_nvs_set_value(const char *key, void* value, nvs_type_t type);
esp_err_t mod_nvs_get_value(const char *key, void* value, nvs_type_t type);
esp_err_t mod_nvs_set_blob(const char *key, void* value, size_t len);
esp_err_t mod_nvs_get_blob(const char *key, void* value, size_t* len);

esp_err_t mod_nvs_erase(const char *key);
esp_err_t mod_nvs_erase_all(void);
int mod_nvs_list(const char *part, const char *name, const char *str_type);