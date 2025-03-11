

#include "cmd_system_sleep.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include "sys/queue.h"

static struct {
    struct arg_str *tag;
    struct arg_str *level;
    struct arg_end *end;
} log_level_args;

typedef struct cmd_item_ {
    const char *command;
    const char *help;
    char *hint;
    esp_console_cmd_func_t func;                        //!< pointer to the command handler (without user context)
    esp_console_cmd_func_with_context_t func_w_context; //!< pointer to the command handler (with user context)
    void *argtable;                                     //!< optional pointer to arg table
    void *context;                                      //!< optional pointer to user context
    SLIST_ENTRY(cmd_item_) next;                        //!< next command in the list
} cmd_item_t;


void cmd_system_register(void);