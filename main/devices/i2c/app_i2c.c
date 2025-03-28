#include "devices/i2c/sensors.h"

#include "devices/display/display.h"

#include "mod_ssd1306.h"
#include "ssd1306_plot.h"
#include "ssd1306_segment.h"
#include "ssd1306_bitmap.h"

#define SSD1306_MAX_PRINTMODE 4

int8_t ssd1306_print_mode = 1;

void ssd1306_set_printMode(uint8_t direction) {
    ssd1306_print_mode += direction;

    if (ssd1306_print_mode < 0) {
        ssd1306_print_mode = 0;
    } else if (ssd1306_print_mode > SSD1306_MAX_PRINTMODE - 1) {
        ssd1306_print_mode = SSD1306_MAX_PRINTMODE - 1;
    }

    printf("print_mode: %d\n", ssd1306_print_mode);
}


// char display_buff[64];

// void on_resolve_bh1750(float lux) {
//     snprintf(display_buff, sizeof(display_buff), "BH1750 %.2f", lux);
//     ssd1306_print_str(display_buff, 2);
// }

// void on_resolve_ap3216(uint16_t ps, uint16_t als) {
//     snprintf(display_buff, sizeof(display_buff), "prox %u, als %u", ps, als);
//     ssd1306_print_str(display_buff, 5);
// }

// void on_resolve_apds9960(uint8_t prox, uint16_t clear,
//     uint16_t red, uint16_t green, uint16_t blue) {
//     snprintf(display_buff, sizeof(display_buff), "ps %u, w %u, r %u, g %u, b %u", prox, clear, red, green, blue);
//     ssd1306_print_str(display_buff, 6);
// }

// void on_resolve_max4400(float lux) {
//     snprintf(display_buff, sizeof(display_buff), "lux %.2f", lux);
//     // ssd1306_print_str(display_buff, 7);
// }

// void on_resolve_vl53lox(uint8_t distance) {
//     snprintf(display_buff, sizeof(display_buff), "dist: %u", distance);
//     // ssd1306_print_str(display_buff, 7);
// }

// void on_resolve_mpu6050(int16_t accel_x, int16_t accel_y, int16_t accel_z) {
//     snprintf(display_buff, sizeof(display_buff), "x %u, y %u, z %u", accel_x, accel_y, accel_z);
//     ssd1306_print_str(display_buff, 4);
// }

// void on_resolve_ina219(int16_t shunt, int16_t bus_mV, int16_t current, int16_t power) {
//     snprintf(display_buff, sizeof(display_buff),"sh %hu, bus %hu, cur %hd, p %hu", 
//                 shunt, bus_mV, current, power);
//     ssd1306_print_str(display_buff, 7);
// }

// void on_resolve_ds3231(ds3231_dateTime_t *datetime) {
//     snprintf(display_buff, sizeof(display_buff), "%u/%u/%u %u:%u:%u", 
//             datetime->month, datetime->date, datetime->year,
//             datetime->hr, datetime->min, datetime->sec);
//     ssd1306_print_str(display_buff, 1);
// }

// void on_resolve_sht31(float temp, float hum) {
//     snprintf(display_buff, sizeof(display_buff), "Temp %.2f, hum %.2f", temp, hum);
//     ssd1306_print_str(display_buff, 3);
// }

void app_i2c_setup(uint8_t scl_pin, uint8_t sda_pin) {
    // display_setup(scl_pin, sda_pin);
    
    // M_Device_Handlers set0_handlers = {
    //     .on_handle_bh1750 = on_resolve_bh1750,
    //     .on_handle_ap3216 = on_resolve_ap3216,
    //     .on_handle_apds9960 = on_resolve_apds9960,
    //     .on_handle_max4400 = on_resolve_max4400,
    //     .on_handle_vl53lox = on_resolve_vl53lox,
    //     .on_handle_mpu6050 = on_resolve_mpu6050,
    //     .on_handle_ina219 = on_resolve_ina219,
    //     .on_handle_ds3231 = on_resolve_ds3231,
    //     .on_handle_sht31 = on_resolve_sht31
    // };

    // M_I2C_Devices_Set i2c_devices_set0;
    // i2c_devices_set0.handlers = &set0_handlers;
    // i2c_devices_setup(&i2c_devices_set0, 0);

    // do_i2cdetect_cmd(scl_pin, sda_pin);
}


uint64_t interval_ref2 = 0;

void app_i2c_task(uint64_t current_time, M_I2C_Devices_Set *devs_set) {
    i2c_sensor_readings(devs_set, current_time);

    // if (current_time - interval_ref2 > 200000) {
    //     interval_ref2 = current_time;
    //     // mod_adc_1read(&mic_adc);
    //     // mod_adc_1read(&pir_adc);
    //     // printf("value = %u\n", pir_adc.value);

    //     // uint8_t value = map_value(mic_adc.value, 1910, 2000, 0, 64);
    //     // printf("raw = %u, value = %u\n", mic_adc.value, value);

    //     if (ssd1306_print_mode == 2) {
    //         ssd1306_spectrum(5);
    //     }
    //     else if (ssd1306_print_mode == 3) {
    //         ssd1306_test_digits();
    //     }
    //     else if (ssd1306_print_mode == 0) {
    //         ssd1306_test_bitmaps();
    //     }
        
    //     // printf("mic reading: %u\n", mic_adc.raw);
    //     // mod_adc_continous_read(&continous_read);
    // }
}