#include "mod_adc.h"


const static char *TAG = "MOD_ADC";

// Function to get ADC1 channel from GPIO pin
static uint8_t gpio_to_adc1_channel(int gpio_num) {
    switch (gpio_num) {
        case 36: return ADC_CHANNEL_0;
        case 37: return ADC_CHANNEL_1;
        case 38: return ADC_CHANNEL_2;
        case 39: return ADC_CHANNEL_3;
        case 32: return ADC_CHANNEL_4;
        case 33: return ADC_CHANNEL_5;
        case 34: return ADC_CHANNEL_6;
        case 35: return ADC_CHANNEL_7;
        default: return 0xFF; // Invalid GPIO
    }
}

// Function to get ADC2 channel from GPIO pin
static uint8_t gpio_to_adc2_channel(int gpio_num) {
    switch (gpio_num) {
        case 4:  return ADC_CHANNEL_0;
        case 0:  return ADC_CHANNEL_1;
        case 2:  return ADC_CHANNEL_2;
        case 15: return ADC_CHANNEL_3;
        case 13: return ADC_CHANNEL_4;
        case 12: return ADC_CHANNEL_5;
        case 14: return ADC_CHANNEL_6;
        case 27: return ADC_CHANNEL_7;
        case 25: return ADC_CHANNEL_8;
        case 26: return ADC_CHANNEL_9;
        default: return 0xFF; // Invalid GPIO
    }
}


/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool adc_calib_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        if (!calibrated) {
            ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
            adc_cali_curve_fitting_config_t cali_config = {
                .unit_id = unit,
                .chan = channel,
                .atten = atten,
                .bitwidth = ADC_BITWIDTH_DEFAULT,
            };
            ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
            if (ret == ESP_OK) {
                calibrated = true;
            }
        }
    #endif

    #if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        if (!calibrated) {
            ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
            adc_cali_line_fitting_config_t cali_config = {
                .unit_id = unit,
                .atten = atten,
                .bitwidth = ADC_BITWIDTH_DEFAULT,
            };
            ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
            if (ret == ESP_OK) {
                calibrated = true;
            }
        }
    #endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void adc_calib_deinit(adc_cali_handle_t handle) {
    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
        ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

    #elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
        ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
    #endif
}

#define ADC_ATTEN   ADC_ATTEN_DB_12

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_calibration_handle = NULL;
static adc_unit_inteface_t sel_unit = ADC_UNIT_INTF_1;
static uint8_t calib_enabled = false;
static uint8_t log_enabled = false;

void mod_adc_1read_init(adc_unit_inteface_t unit, uint8_t calibration, uint8_t log) {
    //! ADC Init
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = unit,                        // ADC1 or ADC2
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    sel_unit = unit;
    calib_enabled = calibration;
    log_enabled = log;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
}

void mod_adc_1read_setup(adc_single_read_t *model) {
    if (adc1_handle == NULL) return;

    uint8_t channel = 255;

    // unit mapping 
    if (sel_unit == ADC_UNIT_INTF_1) {
        channel = gpio_to_adc1_channel(model->gpio);
    } else if (sel_unit == ADC_UNIT_INTF_2) {
        channel = gpio_to_adc2_channel(model->gpio);
    }

    if (channel == 255) {
        adc1_handle = NULL;
        return;
    }
    model->channel = channel;

    //! ADC Config
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, model->channel, &config));

    if (calib_enabled) {
        // ADC Calibration
        adc_calib_init(ADC_UNIT_1, model->channel, ADC_ATTEN, &adc1_calibration_handle);
    }
}

void mod_adc_1read(uint64_t current_time, adc_single_read_t *model) {
    if (adc1_handle == NULL) return;

    int raw_value = 0;
    int adc_number = ADC_UNIT_1 + 1;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, model->channel, &raw_value));

    if (adc1_calibration_handle) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_calibration_handle, raw_value, &model->value));
        if (log_enabled)
            ESP_LOGI(TAG, "ADC%d Chan[%d] Calibrated: %d mV", adc_number, model->channel, model->value);
    } else {
        model->value = raw_value;
        if (log_enabled)
            ESP_LOGI(TAG, "ADC%d Chan[%d] Raw Data: %d", adc_number, model->channel, model->value);
    }
}

void mod_adc_1read_teardown(adc_single_read_t *model) {
    adc_oneshot_del_unit(adc1_handle);

    if (adc1_calibration_handle) {
        adc_calib_deinit(adc1_calibration_handle);
    }
}



#define _ADC_UNIT_STR(unit)     #unit
#define ADC_UNIT_STR(unit)      _ADC_UNIT_STR(unit)
#define EXAMPLE_READ_LEN        256


void mod_adc_continous_setup(adc_continous_read_t *model) {
    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = EXAMPLE_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &model->handle));

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    adc_channel_t channel[2] = {ADC_CHANNEL_6, ADC_CHANNEL_7};
    uint8_t channel_num =  sizeof(channel) / sizeof(adc_channel_t);

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };

    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = ADC_ATTEN_DB_0;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = model->unit;
        adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(model->handle, &dig_cfg));

    // Start ADC continuous mode
    ESP_ERROR_CHECK(adc_continuous_start(model->handle));
    memset(model->results, 0xcc, EXAMPLE_READ_LEN);
}


void mod_adc_continous_read(adc_continous_read_t *model) {
    // Read ADC data
    esp_err_t ret = adc_continuous_read(model->handle, model->results, EXAMPLE_READ_LEN, &model->read_count, 0);

    if (ret == ESP_OK) {
        ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, model->read_count);

        for (int i = 0; i < model->read_count; i += SOC_ADC_DIGI_RESULT_BYTES) {
            adc_digi_output_data_t *p = (adc_digi_output_data_t*)&model->results[i];
            uint32_t chan_num = p->type1.channel;
            uint32_t data = p->type1.data;

            /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
            if (chan_num < SOC_ADC_CHANNEL_NUM(model->unit)) {
                ESP_LOGI(TAG, "Unit: %s, Channel: %"PRIu32", Value: %"PRIx32, ADC_UNIT_STR(model->unit), chan_num, data);
            } else {
                ESP_LOGW(TAG, "Invalid data [%s_%"PRIu32"_%"PRIx32"]", ADC_UNIT_STR(model->unit), chan_num, data);
            }
        }
    } else if (ret == ESP_ERR_TIMEOUT) {
        // No data available
        ESP_LOGW(TAG, "No data available");
    }
}

void mod_adc_continous_teardown(adc_continous_read_t* model) {
    ESP_ERROR_CHECK(adc_continuous_stop(model->handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(model->handle));
}