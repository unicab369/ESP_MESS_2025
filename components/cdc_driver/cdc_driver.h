#ifndef USB_JTAG_H
#define USB_JTAG_H

#include <stdint.h>
#include <stdbool.h>
#include <soc/gpio_num.h>


// Function prototypes
void cdc_setup(void);
void cdc_read_task(void);

#endif