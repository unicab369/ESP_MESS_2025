
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

#define FLAG_MOD_ADC 0

#define EXAMPLE_READ_LEN                    256

typedef enum __attribute__((packed)) {
    ADC_UNIT_INTF_1,
    ADC_UNIT_INTF_2
} adc_unit_inteface_t;


typedef struct {
    uint8_t gpio;
    adc_channel_t channel;
    int value;
} adc_single_read_t;

typedef struct {
    adc_unit_inteface_t unit;
    adc_continuous_handle_t handle;
    uint8_t results[EXAMPLE_READ_LEN];
    uint32_t read_count;
} adc_continous_read_t;

void mod_adc_1read_init(adc_unit_inteface_t unit, uint8_t calibration, uint8_t log);
void mod_adc_1read_setup(adc_single_read_t *model);
void mod_adc_1read(adc_single_read_t *model);
void mod_adc_1read_teardown(adc_single_read_t *model);

void mod_adc_continous_setup(adc_continous_read_t *model);
void mod_adc_continous_read(adc_continous_read_t *model);
void mod_adc_continous_teardown(adc_continous_read_t* model);