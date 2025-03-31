#include <unistd.h>
#include <stdint.h>

#include "mod_spi.h"


void spi_setup_sdCard(uint8_t host, uint8_t cs_pin);
void spi_setup_st7735(M_Spi_Conf *config, uint8_t st7735_cs_pin);
