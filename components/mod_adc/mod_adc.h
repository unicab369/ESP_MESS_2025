
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"

#define EXAMPLE_READ_LEN                    256

typedef enum __attribute__((packed)) {
    ADC_UNIT_INTF_1,
    ADC_UNIT_INTF_2
} adc_unit_inteface_t;


typedef struct {
    uint8_t gpio;
    adc_channel_t channel;
    adc_oneshot_unit_handle_t oneshot_handler;
    adc_cali_handle_t calibration_handler;
    int raw;
    int voltage;
    adc_unit_inteface_t unit;    // ADC1 or ADC2
    bool is_valid;
} adc_single_read_t;

typedef struct {
    adc_unit_inteface_t unit;
    adc_continuous_handle_t handle;
    uint8_t results[EXAMPLE_READ_LEN];
    uint32_t read_count;
} adc_continous_read_t;

void mod_adc_1read_setup(adc_single_read_t *model);
void mod_adc_1read(uint64_t current_time, adc_single_read_t *model);
void mod_adc_1read_teardown(adc_single_read_t *model);

void mod_adc_continous_setup(adc_continous_read_t *model);
void mod_adc_continous_read(adc_continous_read_t *model);
void mod_adc_continous_teardown(adc_continous_read_t* model);