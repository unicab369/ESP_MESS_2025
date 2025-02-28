#include "behavior.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include "esp_log.h"
#include "littlefs/littlefs.h"

behavior_config_t device_behaviors[10];

void behavior_setup(uint8_t* esp_mac) {


    littlefs_setup();

}

