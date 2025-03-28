#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "mod_i2c.h"

void ssd1306_spectrum(M_I2C_Device *device, uint8_t num_band);