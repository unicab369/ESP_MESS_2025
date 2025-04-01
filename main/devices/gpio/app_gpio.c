
#include "app_gpio.h"
#include "mod_ws2812.h"

#include "mod_adc.h"

void app_gpio_ws2812(uint8_t pin) {
    ws2812_setup(pin);

    ws2812_cyclePulse_t ojb1 = {
        .obj_index = 0,
        .led_index = 4,
        .rgb = { .red = 150, .green = 0, .blue = 0 },
        .config = {
            .pulse_count = 1,
            .pulse_time_ms = 100,
            .wait_time_ms = 1000,
        }
    };
    // ws2812_load_pulse(ojb1);

    ws2812_cyclePulse_t ojb2 = {
        .obj_index = 1,
        .led_index = 3,
        .rgb =  { .red = 0, .green = 150, .blue = 0 },
        .config = {
            .pulse_count = 2,
            .pulse_time_ms = 100,
            .wait_time_ms = 1000,
        }
    };
    // ws2812_load_pulse(ojb2);

    ws2812_cycleFade_t obj3 = {
        .led_index = 1,
        .active_channels = { .red = 0xFF, .green = 0, .blue = 0xFF },
        .config = {
            .current_value = 0,
            .direction = true,
            .is_bounced = true,
            .increment = 5,
            .start_index = 0,
            .end_index = 150,       // hue max 360
            .refresh_time_uS = 20000,
            .last_refresh_time = 0
        }
    };
    // ws2812_load_fadeColor(obj3, 0);

    ws2812_cycleFade_t obj4 = {
        .led_index = 2,
        .active_channels = { .red = 0xFF, .green = 0, .blue = 0 },
        .config = {
            .current_value = 0,
            .direction = true,
            .is_bounced = false,
            .increment = 5,
            .start_index = 0,
            .end_index = 150,       // hue max 360
            .refresh_time_uS = 50000,
            .last_refresh_time = 0
        }
    };
    // ws2812_load_fadeColor(obj4, 1);

    // mod_mbedtls_setup();
    
    // data_output_t data_output = {
    //     .type = DATA_OUTPUT_ANALOG,
    //     .len = 100
    // };
}

adc_single_read_t pir_adc = {
    .gpio = 35,
};

void app_gpio_set_adc() {
    // adc_single_read_t single_adc = {
    //     .gpio = GPIO_35,
    // };
    // mod_adc_1read_setup(&single_adc);    

    // adc_single_read_t mic_adc = {
    //     .gpio = MIC_IN,
    // };
    // mod_adc_1read_setup(&mic_adc);

    mod_adc_1read_setup(&pir_adc);

    // adc_continous_read_t continous_read = (adc_continous_read_t){
    //     .unit = ADC_UNIT_1
    // };

    // mod_adc_continous_setup(&continous_read);
}

static uint64_t interval_ref = 0;

void app_gpio_task(uint64_t current_time) {
    if (current_time - interval_ref > 200000) {
        interval_ref = current_time;
        // mod_adc_1read(&mic_adc);
        mod_adc_1read(&pir_adc);
        // printf("value = %u\n", pir_adc.value);

        // uint8_t value = map_value(mic_adc.value, 1910, 2000, 0, 64);
        // printf("raw = %u, value = %u\n", mic_adc.value, value);
        
        // printf("mic reading: %u\n", mic_adc.raw);
        // mod_adc_continous_read(&continous_read);

        // printf("single_adc: %u\n", single_adc.value);
    }

    ws2812_loop(current_time);
}