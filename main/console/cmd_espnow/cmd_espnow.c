#include "cmd_espnow.h"

#include "esp_now.h"
#include "esp_log.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

static struct {
    struct arg_str *key;
    struct arg_str *type;
    struct arg_str *value;
    struct arg_end *end;
} set_args;


static int set_value(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &set_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, set_args.end, argv[0]);
        return 1;
    }

    const char *key = set_args.key->sval[0];
    const char *str_type = set_args.type->sval[0];
    const char *str_value = set_args.value->sval[0];

    // nvs_type_t type = str_to_type(str_type);
    // esp_err_t err = ESP_FAIL;

    // if (type == NVS_TYPE_ANY) {
    //     ESP_LOGE(TAG, "Type '%s' is undefined", str_type);
    //     return ESP_ERR_NVS_TYPE_MISMATCH;
    // } else if (type == NVS_TYPE_BLOB) {
    //     err = store_blob(key, str_value);
    // } else if (type == NVS_TYPE_STR) {
    //     err = mod_nvs_set_value(key, str_value, type);
    // } else {
    //     int64_t value = strtol(str_value, NULL, 0);
    //     err = mod_nvs_set_value(key, &value, type);
    // }

    // return check_esp_ok(err);
}


void cmd_espnow_setup(void) {
    const esp_console_cmd_t set_cmd = {
        .command = "nvs_set",
        .help = "Set key-value pair in selected namespace.\n"
        "Examples:\n"
        " nvs_set VarName i32 -v 123 \n"
        " nvs_set VarName str -v YourString \n"
        " nvs_set VarName blob -v 0123456789abcdef \n",
        .hint = NULL,
        .func = &set_value,
        .argtable = &set_args
    };
}