#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <soc/gpio_num.h>

void ws2812_setup(void);
void ws2812_run1(void);
void ws2812_loop(void);
void ws2812_loop2(void);

#endif