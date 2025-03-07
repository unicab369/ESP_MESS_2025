#include <stdint.h>

typedef struct {
    uint8_t mosi;
    uint8_t miso;
    uint8_t sclk;
    uint8_t cs;
} storage_sd_config_t;

void storage_sd_configure(storage_sd_config_t *config);
void storage_sd_test(void);