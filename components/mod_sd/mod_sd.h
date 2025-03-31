#include <stdint.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "sdmmc_cmd.h"

void mod_sd_spi_config(uint8_t spi_host, uint8_t cs_pin);

void mod_sd_mmc_config(int8_t clk, int8_t di, int8_t d0, int8_t d1, int8_t d2, int8_t d3);
void mod_sd_test(void);

esp_err_t mod_sd_fopen(const char *path);
size_t mod_sd_fread(char *buff, size_t len);
int mod_sd_fclose();

esp_err_t mod_sd_get(const char *path, char *buffer, size_t len);
esp_err_t mod_sd_write(const char *path, char *data);
