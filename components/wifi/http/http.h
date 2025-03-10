#include <stdint.h>

#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"

typedef struct {
    esp_err_t(*on_file_fopen_cb)(const char *path);
    size_t(*on_file_fread_cb)(const char *path, char *buffer, size_t len);
    int(*on_file_fclose_cb)(void);
} http_interface_t;

void http_setup(http_interface_t* interface);
void http_get_request(void);
void http_post_request(void);