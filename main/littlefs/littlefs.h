#ifndef MYLITTLEFS_DRIVER_H
#define MYLITTLEFS_DRIVER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef void (*littlefs_readfile_cb)(char* file_name, char* buff, size_t len);

// Function prototypes
void littlefs_setup(void);
void littlefs_test(void);
void littlefs_loadFiles(const char* path, littlefs_readfile_cb callback);

#endif