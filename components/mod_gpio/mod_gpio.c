#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"

typedef struct {
    uint8_t gpio;
    gpio_mode_t mode;
    bool is_pullup;
    bool is_input;
    bool is_output;
} app_gpio_config_t;

static bool is_input(gpio_mode_t mode) {
    return mode == GPIO_MODE_INPUT 
        || mode == GPIO_MODE_INPUT_OUTPUT 
        || mode == GPIO_MODE_INPUT_OUTPUT_OD;
}

static bool is_output(gpio_mode_t mode) {
    return mode == GPIO_MODE_OUTPUT 
        || mode == GPIO_MODE_INPUT_OUTPUT
        || mode == GPIO_MODE_INPUT_OUTPUT_OD;
}

void app_gpio_configure(app_gpio_config_t *config) {
    config->is_input = is_input(config->mode);
    config->is_output = is_output(config->mode);
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->gpio),
        .mode = config->mode,
        .pull_up_en = config->is_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

int app_gpio_read(app_gpio_config_t *config) {
    if (!config->is_input) return 0;
    return gpio_get_level(config->gpio);
}

void app_gpio_set(app_gpio_config_t *config, bool state) {
    if (!config->is_output) return;
    gpio_set_level(config->gpio, state);
}

void app_gpio_toggle(app_gpio_config_t *config) {
    if (!config->is_output) return;
    gpio_set_level(config->gpio, !gpio_get_level(config->gpio));
}