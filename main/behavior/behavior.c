#include "behavior.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include "esp_log.h"
#include "littlefs/littlefs.h"

behavior_config_t device_behaviors[10];

void littlefs_readfile_handler(char* file_name, char* buff, size_t len) {
    printf("%s", buff); // Print each line of the file
    printf("\n");
}

void behavior_setup(uint8_t* esp_mac) {
    littlefs_setup();
    littlefs_loadFiles("/littlefs/devices", littlefs_readfile_handler);
}

