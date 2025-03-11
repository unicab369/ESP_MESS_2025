/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Console example — various system commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "cmd_system.h"

#include "esp_log.h"
#include "esp_console.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"



static const char *TAG = "cmd_system_common";


static int get_stats(int argc, char **argv)
{
    const char *model;
    esp_chip_info_t info;
    uint32_t flash_size;
    esp_chip_info(&info);

    switch(info.model) {
        case CHIP_ESP32:        model = "ESP32"; break;
        case CHIP_ESP32S2:      model = "ESP32-S2"; break;
        case CHIP_ESP32S3:      model = "ESP32-S3"; break;
        case CHIP_ESP32C3:      model = "ESP32-C3"; break;
        case CHIP_ESP32H2:      model = "ESP32-H2"; break;
        case CHIP_ESP32C2:      model = "ESP32-C2"; break;
        case CHIP_ESP32P4:      model = "ESP32-P4"; break;
        case CHIP_ESP32C5:      model = "ESP32-C5"; break;
        default:                model = "Unknown"; break;
    }

    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
    } else {
        printf("IDF Version:%s\r\n", esp_get_idf_version());
        printf("Chip info:\r\n");
        printf("\tmodel:%s\r\n", model);
        printf("\tcores:%d\r\n", info.cores);
        printf("\tfeature:%s%s%s%s%"PRIu32"%s\r\n",
            info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
            info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
            info.features & CHIP_FEATURE_BT ? "/BT" : "",
            info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:",
            flash_size / (1024 * 1024), " MB");
        printf("\trevision number:%d\r\n", info.revision);
    }

    uint32_t heap_size = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    printf("min heap size: %"PRIu32"\n", heap_size);
    printf("free heap: %"PRIu32"\n", esp_get_free_heap_size());

    return 0;
}

static int restart(int argc, char **argv)
{
    ESP_LOGI(TAG, "Restarting");
    esp_restart();
}

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
static int tasks_info(int argc, char **argv) {
    const size_t bytes_per_task = 40; /* see vTaskList description */
    char *task_list_buffer = malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
    if (task_list_buffer == NULL) {
        ESP_LOGE(TAG, "failed to allocate buffer for vTaskList output");
        return 1;
    }
    fputs("Task Name\tStatus\tPrio\tHWM\tTask#", stdout);
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
    fputs("\tAffinity", stdout);
#endif
    fputs("\n", stdout);
    vTaskList(task_list_buffer);
    fputs(task_list_buffer, stdout);
    free(task_list_buffer);
    return 0;
}
#endif




static const char* s_log_level_names[] = {
    "none", "error", "warn", "info", "debug", "verbose"
};

static int log_level(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &log_level_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, log_level_args.end, argv[0]);
        return 1;
    }
    assert(log_level_args.tag->count == 1);
    assert(log_level_args.level->count == 1);
    const char* tag = log_level_args.tag->sval[0];
    const char* level_str = log_level_args.level->sval[0];
    esp_log_level_t level;
    size_t level_len = strlen(level_str);
    for (level = ESP_LOG_NONE; level <= ESP_LOG_VERBOSE; level++) {
        if (memcmp(level_str, s_log_level_names[level], level_len) == 0) {
            break;
        }
    }
    if (level > ESP_LOG_VERBOSE) {
        printf("Invalid log level '%s', choose from none|error|warn|info|debug|verbose\n", level_str);
        return 1;
    }
    if (level > CONFIG_LOG_MAXIMUM_LEVEL) {
        printf("Can't set log level to %s, max level limited in menuconfig to %s. "
            "Please increase CONFIG_LOG_MAXIMUM_LEVEL in menuconfig.\n",
            s_log_level_names[level], s_log_level_names[CONFIG_LOG_MAXIMUM_LEVEL]);
        return 1;
    }
    esp_log_level_set(tag, level);
    return 0;
}


typedef void (*const fn_print_arg_t)(cmd_item_t*);

void cmd_system_register(void) {
    esp_console_register_help_command();        /* Register commands */

    const esp_console_cmd_t cmd_get_stats = {
        .command = "stats",
        .help = "Get system statistics",
        .hint = NULL,
        .func = &get_stats,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_get_stats) );

    const esp_console_cmd_t cmd_restart = {
        .command = "restart",
        .help = "Software reset of the chip",
        .hint = NULL,
        .func = &restart,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_restart) );

    log_level_args.tag = arg_str1(NULL, NULL, "<tag|*>", "Log tag to set the level for, or * to set for all tags");
    log_level_args.level = arg_str1(NULL, NULL, "<none|error|warn|debug|verbose>", "Log level to set. Abbreviated words are accepted.");
    log_level_args.end = arg_end(2);

    const esp_console_cmd_t cmd_log_level = {
        .command = "log_level",
        .help = "Set log level for all tags or a specific tag.",
        .hint = NULL,
        .func = &log_level,
        .argtable = &log_level_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_log_level) );

    #ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
        const esp_console_cmd_t cmd_tasks = {
            .command = "tasks",
            .help = "Get information about running tasks",
            .hint = NULL,
            .func = &tasks_info,
        };
        ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_tasks) );
    #endif

    #if SOC_LIGHT_SLEEP_SUPPORTED
        cmd_system_register_light_sleep();
    #endif
    #if SOC_DEEP_SLEEP_SUPPORTED
        cmd_system_register_deep_sleep();
    #endif
}
