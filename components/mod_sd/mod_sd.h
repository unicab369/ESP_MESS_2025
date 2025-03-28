#include <stdint.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "sdmmc_cmd.h"

typedef struct {
    bool mmc_enabled;
    struct {
        bool enable_width4;         // bus width
        uint8_t clk;
        uint8_t di;
        uint8_t d0;
        uint8_t d1;
        uint8_t d2;
        uint8_t d3;
    } mmc;

} storage_sd_config_t;

void mod_sd_spi_config(uint8_t cs_pin);

void mod_sd_mmc_config(storage_sd_config_t *config);
void mod_sd_test(void);

esp_err_t mod_sd_fopen(const char *path);
size_t mod_sd_fread(char *buff, size_t len);
int mod_sd_fclose();

esp_err_t mod_sd_get(const char *path, char *buffer, size_t len);
esp_err_t mod_sd_write(const char *path, char *data);
