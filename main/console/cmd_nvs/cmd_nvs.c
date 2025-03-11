/* Console example — NVS commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "cmd_nvs.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "nvs.h"
#include "mod_nvs.h"

static const size_t TYPE_STR_PAIR_SIZE = sizeof(type_str_pair) / sizeof(type_str_pair[0]);
static const char *ARG_TYPE_STR = "type can be: i8, u8, i16, u16 i32, u32 i64, u64, str, blob";
static const char *TAG = "cmd_nvs";



static nvs_type_t str_to_type(const char *type) {
    for (int i = 0; i < TYPE_STR_PAIR_SIZE; i++) {
        const type_str_pair_t *p = &type_str_pair[i];
        if (strcmp(type, p->str) == 0) {
            return  p->type;
        }
    }

    return NVS_TYPE_ANY;
}

static esp_err_t store_blob(const char *key, const char *str_values) {
    uint8_t value;
    size_t str_len = strlen(str_values);
    size_t blob_len = str_len / 2;

    if (str_len % 2) {
        ESP_LOGE(TAG, "Blob data must contain even number of characters");
        return ESP_ERR_NVS_TYPE_MISMATCH;
    }

    char *blob = (char *)malloc(blob_len);
    if (blob == NULL) {
        return ESP_ERR_NO_MEM;
    }

    for (int i = 0, j = 0; i < str_len; i++) {
        char ch = str_values[i];
        if (ch >= '0' && ch <= '9') {
            value = ch - '0';
        } else if (ch >= 'A' && ch <= 'F') {
            value = ch - 'A' + 10;
        } else if (ch >= 'a' && ch <= 'f') {
            value = ch - 'a' + 10;
        } else {
            ESP_LOGE(TAG, "Blob data contain invalid character");
            free(blob);
            return ESP_ERR_NVS_TYPE_MISMATCH;
        }

        if (i & 1) {
            blob[j++] += value;
        } else {
            blob[j] = value << 4;
        }
    }

    esp_err_t err = mod_nvs_set_blob(key, blob, blob_len);
    free(blob);
    return err;
}

static int check_esp_ok(esp_err_t err) {
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s", esp_err_to_name(err));
        return 1;
    }

    return 0;
}

static int check_arg(int argc, char **argv, void **argtable, struct arg_end *end) {
    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors != 0) arg_print_errors(stderr, end, argv[0]);
    return nerrors;
}

static int set_value(int argc, char **argv) {
    if (check_arg(argc, argv, (void**) &set_args, set_args.end) != 0) return 1;

    const char *key = set_args.key->sval[0];
    const char *str_type = set_args.type->sval[0];
    const char *str_value = set_args.value->sval[0];

    nvs_type_t type = str_to_type(str_type);
    esp_err_t err = ESP_FAIL;

    if (type == NVS_TYPE_ANY) {
        ESP_LOGE(TAG, "Type '%s' is undefined", str_type);
        return ESP_ERR_NVS_TYPE_MISMATCH;
    } else if (type == NVS_TYPE_BLOB) {
        err = store_blob(key, str_value);
    } else if (type == NVS_TYPE_STR) {
        err = mod_nvs_set_value(key, str_value, type);
    } else {
        int64_t value = strtol(str_value, NULL, 0);
        err = mod_nvs_set_value(key, &value, type);
    }

    return check_esp_ok(err);
}

static int get_value(int argc, char **argv) {
    if (check_arg(argc, argv, (void**) &get_args, get_args.end) != 0) return 1;

    const char *key = get_args.key->sval[0];
    const char *str_type = get_args.type->sval[0];

    nvs_type_t type = str_to_type(str_type);
    esp_err_t err = ESP_FAIL;

    if (type == NVS_TYPE_ANY) {
        ESP_LOGE(TAG, "Type '%s' is undefined", str_type);
        return ESP_ERR_NVS_TYPE_MISMATCH;

    } else if (type == NVS_TYPE_BLOB) {
        size_t len = 0;
        uint8_t *blob = mod_nvs_get_blob(key, &len);

        for (int i = 0; i < len; i++) {
            printf("%02x", blob[i]);
        }
        printf("\n");
        if (len>0) err = ESP_OK;

    } else if (type == NVS_TYPE_STR) {
        char str[32];
        err = mod_nvs_get_value(key, &str, type);
        ESP_LOGI(TAG, "str = %s", str);

    } else {
        // cheating
        if (type == NVS_TYPE_I64 || type == NVS_TYPE_U64) {
            int64_t value;
            err = mod_nvs_get_value(key, &value, type);    
            ESP_LOGI(TAG, "value = %" PRId64, value);

        } else if (type == NVS_TYPE_I32 || type == NVS_TYPE_U32) {
            int32_t value;
            err = mod_nvs_get_value(key, &value, type);
            ESP_LOGI(TAG, "value = %" PRId32, value);

        } else if (type == NVS_TYPE_I16 || type == NVS_TYPE_U16) {
            int16_t value;
            err = mod_nvs_get_value(key, &value, type);    
            ESP_LOGI(TAG, "value = %" PRId16, value);

        } else {
            uint8_t value;
            err = mod_nvs_get_value(key, &value, type);    
            ESP_LOGI(TAG, "value = %u", value);
        }
        
    }

    return check_esp_ok(err);
}

static int erase_value(int argc, char **argv) {
    if (check_arg(argc, argv, (void**) &erase_args, erase_args.end) != 0) return 1;

    const char *key = erase_args.key->sval[0];
    esp_err_t err = mod_nvs_erase(key);
    return check_esp_ok(err);
}

static int erase_selected_namespace(int argc, char **argv) {
    if (check_arg(argc, argv, (void**) &erase_all_args, erase_all_args.end) != 0) return 1;

    esp_err_t err = mod_nvs_erase_all();
    return check_esp_ok(err);
}

static int set_namespace(int argc, char **argv) {
    if (check_arg(argc, argv, (void**) &namespace_args, namespace_args.end) != 0) return 1;

    const char *namespace = namespace_args.namespace->sval[0];
    mod_nvs_set_namespace(namespace);
    return 0;
}

static int list_entries(int argc, char **argv) {
    if (check_arg(argc, argv, (void**) &list_args, list_args.end) != 0) return 1;
    
    list_args.partition->sval[0] = "";
    list_args.namespace->sval[0] = "";
    list_args.type->sval[0] = "";

    const char *partition = list_args.partition->sval[0];
    const char *name = list_args.namespace->sval[0];
    const char *type_str = list_args.type->sval[0];

    nvs_type_t type = str_to_type(type_str);
    return mod_nvs_list(partition, name, type);
}

void register_nvs(void)
{
    // mod_nvs_setup();

    set_args.key = arg_str1(NULL, NULL, "<key>", "key of the value to be set");
    set_args.type = arg_str1(NULL, NULL, "<type>", ARG_TYPE_STR);
    set_args.value = arg_str1("v", "value", "<value>", "value to be stored");
    set_args.end = arg_end(2);

    get_args.key = arg_str1(NULL, NULL, "<key>", "key of the value to be read");
    get_args.type = arg_str1(NULL, NULL, "<type>", ARG_TYPE_STR);
    get_args.end = arg_end(2);

    erase_args.key = arg_str1(NULL, NULL, "<key>", "key of the value to be erased");
    erase_args.end = arg_end(2);

    erase_all_args.namespace = arg_str1(NULL, NULL, "<namespace>", "namespace to be erased");
    erase_all_args.end = arg_end(2);

    namespace_args.namespace = arg_str1(NULL, NULL, "<namespace>", "namespace of the partition to be selected");
    namespace_args.end = arg_end(2);

    list_args.partition = arg_str1(NULL, NULL, "<partition>", "partition name");
    list_args.namespace = arg_str0("n", "namespace", "<namespace>", "namespace name");
    list_args.type = arg_str0("t", "type", "<type>", ARG_TYPE_STR);
    list_args.end = arg_end(2);

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

    const esp_console_cmd_t get_cmd = {
        .command = "nvs_get",
        .help = "Get key-value pair from selected namespace. \n"
        "Example: nvs_get VarName i32",
        .hint = NULL,
        .func = &get_value,
        .argtable = &get_args
    };

    const esp_console_cmd_t erase_cmd = {
        .command = "nvs_erase",
        .help = "Erase key-value pair from current namespace",
        .hint = NULL,
        .func = &erase_value,
        .argtable = &erase_args
    };

    const esp_console_cmd_t erase_namespace_cmd = {
        .command = "nvs_erase_namespace",
        .help = "Erases specified namespace",
        .hint = NULL,
        .func = &erase_selected_namespace,
        .argtable = &erase_all_args
    };

    const esp_console_cmd_t namespace_cmd = {
        .command = "nvs_namespace",
        .help = "Set current namespace",
        .hint = NULL,
        .func = &set_namespace,
        .argtable = &namespace_args
    };

    const esp_console_cmd_t list_entries_cmd = {
        .command = "nvs_list",
        .help = "List stored key-value pairs stored in NVS."
        "Namespace and type can be specified to print only those key-value pairs.\n"
        "Following command list variables stored inside 'nvs' partition, under namespace 'storage' with type uint32_t"
        "Example: nvs_list nvs -n storage -t u32 \n",
        .hint = NULL,
        .func = &list_entries,
        .argtable = &list_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&set_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&get_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&erase_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&namespace_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&list_entries_cmd));
    ESP_ERROR_CHECK(esp_console_cmd_register(&erase_namespace_cmd));
}
