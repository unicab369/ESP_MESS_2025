#include "cmd_espnow.h"

#include "esp_now.h"
#include "esp_log.h"
#include "esp_console.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

#include "mod_espnow.h"

static const char *TAG = "CMD_ESPNOW";

static struct {
    struct arg_str *key;
    struct arg_str *value;
    struct arg_end *end;
} msg_args;


static int check_arg(int argc, char **argv, void **argtable, struct arg_end *end) {
    int nerrors = arg_parse(argc, argv, argtable);
    if (nerrors != 0) arg_print_errors(stderr, end, argv[0]);
    return nerrors;
}

static int check_esp_ok(esp_err_t err) {
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s", esp_err_to_name(err));
        return 1;
    }
    return 0;
}

static int handle_espnow_send(int argc, char **argv) {
    if (check_arg(argc, argv, (void**) &msg_args, msg_args.end) != 0) return 1;

    const char *key = msg_args.key->sval[0];
    const char *str_value = msg_args.value->sval[0];

    uint8_t dest_mac[6] = { 0xAA };
    uint8_t data[32] = { 0xBB };

    espnow_message_t message = {
        .access_code = 33,
        .group_id = 11,
        .msg_id = 12,
        .time_to_live = 15,
    };

    memcpy(message.target_addr, dest_mac, sizeof(message.target_addr));
    memcpy(message.data, data, sizeof(message.data));
    espnow_send((uint8_t*)&message, sizeof(message));

    // nvs_type_t type = str_to_type(str_type);
    // esp_err_t err = ESP_FAIL;

    // return check_esp_ok(err);
    return 0;
}


void cmd_espnow_setup(void) {
    msg_args.key = arg_str1(NULL, NULL, "<key>", "key of the value to be set");
    msg_args.value = arg_str1("v", "value", "<value>", "value to be stored");
    msg_args.end = arg_end(2);

    const esp_console_cmd_t esp_send_cmd = {
        .command = "enow_send",
        .help = "Set key-value pair in selected namespace.\n"
        "Examples:\n"
        " nvs_set VarName i32 -v 123 \n"
        " nvs_set VarName str -v YourString \n"
        " nvs_set VarName blob -v 0123456789abcdef \n",
        .hint = NULL,
        .func = &handle_espnow_send,
        .argtable = &msg_args
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&esp_send_cmd) );
}