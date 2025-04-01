#include "app_serial.h"

#include "freertos/queue.h"

#include "mod_ssd1306.h"
#include "ssd1306_plot.h"
#include "ssd1306_segment.h"
#include "ssd1306_bitmap.h"

static const char *TAG = "APP_SERIAL";

#define MAX_PRINT_MODE 4
#define MAX_PRINT_QUEUE 15

typedef struct {
    char text[32];
    uint8_t x, y;
    uint8_t line
} M_Print;

M_I2C_Devices_Set devices_set0, devices_set1;
QueueHandle_t msg_queue;
bool has_set0 = false;
bool has_set1 = false;

int8_t ssd1306_print_mode = 1;
char display_buff[64];

void app_serial_setMode(uint8_t direction) {
    ssd1306_print_mode += direction;

    if (ssd1306_print_mode < 0) {
        ssd1306_print_mode = 0;
    } else if (ssd1306_print_mode > MAX_PRINT_MODE - 1) {
        ssd1306_print_mode = MAX_PRINT_MODE - 1;
    }

    printf("print_mode: %d\n", ssd1306_print_mode);
}

void app_serial_add_print(const char* buff, uint8_t line) {
    if (ssd1306_print_mode != 1) return;

    M_Print msg = { .line = line };
    strncpy(msg.text, buff, sizeof(msg.text));
    
    BaseType_t stat = xQueueSend(msg_queue, &msg, 0);
    if (stat == errQUEUE_FULL) {
        ESP_LOGW(TAG, "Queue is full");
    } else if (stat != pdPASS) {
        ESP_LOGW(TAG, "Queue failed");
    }
    
    // ssd1306_print_str(devices_set0.ssd1306, buff, line);
    // ssd1306_print_str(devices_set1.ssd1306, buff, line);
}

static void on_resolve_bh1750(float lux) {
    snprintf(display_buff, sizeof(display_buff), "BH1750 %.2f", lux);
    app_serial_add_print(display_buff, 2);
}

static void on_resolve_ap3216(uint16_t ps, uint16_t als) {
    snprintf(display_buff, sizeof(display_buff), "prox %u, als %u", ps, als);
    app_serial_add_print(display_buff, 5);
}

static void on_resolve_apds9960(uint8_t prox, uint16_t clear,
    uint16_t red, uint16_t green, uint16_t blue) {
    snprintf(display_buff, sizeof(display_buff), "ps %u, w %u, r %u, g %u, b %u", prox, clear, red, green, blue);
    app_serial_add_print(display_buff, 6);
}

static void on_resolve_max4400(float lux) {
    snprintf(display_buff, sizeof(display_buff), "lux %.2f", lux);
    // app_serial_add_print(display_buff, 7);
}

static void on_resolve_vl53lox(uint8_t distance) {
    snprintf(display_buff, sizeof(display_buff), "dist: %u", distance);
    // app_serial_add_print(display_buff, 7);
}

static void on_resolve_mpu6050(int16_t accel_x, int16_t accel_y, int16_t accel_z) {
    snprintf(display_buff, sizeof(display_buff), "x %u, y %u, z %u", accel_x, accel_y, accel_z);
    app_serial_add_print(display_buff, 4);
}

static void on_resolve_ina219(int16_t shunt, int16_t bus_mV, int16_t current, int16_t power) {
    snprintf(display_buff, sizeof(display_buff),"sh %hu, bus %hu, cur %hd, p %hu", 
                shunt, bus_mV, current, power);
                app_serial_add_print(display_buff, 7);
}

static void on_resolve_ds3231(ds3231_dateTime_t *datetime) {
    snprintf(display_buff, sizeof(display_buff), "%u/%u/%u %u:%u:%u", 
            datetime->month, datetime->date, datetime->year,
            datetime->hr, datetime->min, datetime->sec);
    app_serial_add_print(display_buff, 1);
}

static void on_resolve_sht31(float temp, float hum) {
    snprintf(display_buff, sizeof(display_buff), "Temp %.2f, hum %.2f", temp, hum);
    app_serial_add_print(display_buff, 3);
}

M_Device_Handlers device_handlers = {
    .on_handle_bh1750 = on_resolve_bh1750,
    .on_handle_ap3216 = on_resolve_ap3216,
    .on_handle_apds9960 = on_resolve_apds9960,
    .on_handle_max4400 = on_resolve_max4400,
    .on_handle_vl53lox = on_resolve_vl53lox,
    .on_handle_mpu6050 = on_resolve_mpu6050,
    .on_handle_ina219 = on_resolve_ina219,
    .on_handle_ds3231 = on_resolve_ds3231,
    .on_handle_sht31 = on_resolve_sht31
};

void app_serial_i2c_setup(uint8_t scl_pin, uint8_t sda_pin, uint8_t port) {
    //# Setup i2C Bus
    esp_err_t ret = i2c_setup(scl_pin, sda_pin, port);
    if (ret != ESP_OK) return;

    msg_queue = xQueueCreate(MAX_PRINT_QUEUE, sizeof(M_Print));
    if (port == 0) {
        devices_set0.handlers = &device_handlers;
        i2c_devices_setup(&devices_set0, port);
        has_set0 = true;
    
    } else if (port == 1) {
        devices_set1.handlers = &device_handlers;
        i2c_devices_setup(&devices_set1, port);
        has_set1 = true;
    }
    
}

uint64_t time_ref = 0;
uint64_t print_timeRef = 0;

static void handle_task(uint64_t current_time, M_I2C_Devices_Set *devs_set) {
    i2c_sensor_readings(devs_set, current_time);

    M_Print msg;
    if (xQueueReceive(msg_queue, &msg, 1) == pdTRUE) {
        ssd1306_print_str(devs_set->ssd1306, msg.text, msg.line);
    }

    // if (current_time - time_ref < 100000) return;
    // time_ref = current_time;

    // if (ssd1306_print_mode == 2) {
    //     ssd1306_spectrum(devs_set->ssd1306, 5);
    // }
    // else if (ssd1306_print_mode == 3) {
    //     ssd1306_test_digits(devs_set->ssd1306);
    // }
    // else if (ssd1306_print_mode == 0) {
    //     ssd1306_test_bitmaps(devs_set->ssd1306);
    // }
}

void app_serial_i2c_task(uint64_t current_time) {
    if (has_set0) {
        handle_task(current_time, &devices_set0);
    }
    if (has_set1) {
        handle_task(current_time, &devices_set1);
    }
}