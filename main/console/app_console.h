#ifndef APP_CONSOLE_H
#define APP_CONSOLE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define FLAG_APP_CONSOLE 0

void app_console_setup(void);
void app_console_deinit(void);
void app_console_task(void);

#endif