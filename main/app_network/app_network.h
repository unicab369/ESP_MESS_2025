
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

typedef enum __attribute__((packed)) {
    DATA_OUTPUT_ANALOG,
    DATA_OUTPUT_DIGITAL,
    DATA_OUTPUT_BUTTON,
    DATA_OUTPUT_ROTARYDIAL,
    DATA_OUTPUT_I2C
} data_output_e;

typedef struct {
    data_output_e type;
    uint16_t len;
    uint16_t *data;
} data_output_t;

void app_network_setup(void);
void app_network_task(uint64_t current_time);
void app_network_push_data(data_output_t data_output);