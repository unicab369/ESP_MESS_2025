

#include "esp_log.h"
#include "esp_console.h"
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"
#include "sdkconfig.h"

static struct {
    struct arg_int *wakeup_time;
#if SOC_PM_SUPPORT_EXT0_WAKEUP || SOC_PM_SUPPORT_EXT1_WAKEUP
    struct arg_int *wakeup_gpio_num;
    struct arg_int *wakeup_gpio_level;
#endif
    struct arg_end *end;
} deep_sleep_args;

static struct {
    struct arg_int *wakeup_time;
    struct arg_int *wakeup_gpio_num;
    struct arg_int *wakeup_gpio_level;
    struct arg_end *end;
} light_sleep_args;


void cmd_system_register_deep_sleep(void);
void cmd_system_register_light_sleep(void);