#include "espnow_controller.h"
#include "espnow_driver.h"
#include <string.h>
#include "esp_timer.h"
#include "esp_log.h"

void espnow_controller_setup() {
    espnow_setup();
}

void espnow_controller_send() {
    espnow_send();
}