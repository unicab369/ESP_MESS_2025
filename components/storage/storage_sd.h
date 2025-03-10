#include <stdint.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "sdmmc_cmd.h"

typedef struct {
    uint8_t mosi;
    uint8_t miso;
    uint8_t sclk;
    uint8_t cs;
} storage_sd_config_t;

void storage_sd_configure(storage_sd_config_t *config);
void storage_sd_test(void);

esp_err_t storage_sd_fopen(const char *path);
size_t storage_sd_fread(const char *path, char *buff, size_t len);
int storage_sd_fclose();

esp_err_t storage_sd_get(const char *path, char *buffer, size_t len);
esp_err_t storage_sd_write(const char *path, char *data);
