
#include "sx127x/sx127x.c"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"

#include "mod_spi.h"
#include "driver/gpio.h"

static const char *TAG = "MOD_SX127";

#define DIO0 13
#define RST 17

sx127x device;

void lora_rx_callback(sx127x *device, uint8_t *data, uint16_t data_length) {
    uint8_t payload[514];
    const char SYMBOLS[] = "0123456789ABCDEF";
    for (size_t i = 0; i < data_length; i++) {
        uint8_t cur = data[i];
        payload[2 * i] = SYMBOLS[cur >> 4];
        payload[2 * i + 1] = SYMBOLS[cur & 0x0F];
    }
    payload[data_length * 2] = '\0';

    int16_t rssi;
    ESP_ERROR_CHECK(sx127x_rx_get_packet_rssi(device, &rssi));
    float snr;
    ESP_ERROR_CHECK(sx127x_lora_rx_get_packet_snr(device, &snr));
    int32_t frequency_error;
    ESP_ERROR_CHECK(sx127x_rx_get_frequency_error(device, &frequency_error));
    ESP_LOGI(TAG, "received: %d %s rssi: %d snr: %f freq_error: %" PRId32, data_length, payload, rssi, snr, frequency_error);
}

void cad_callback(sx127x *device, int cad_detected) {
    if (cad_detected == 0) {
        ESP_LOGI(TAG, "cad not detected");
        ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_CAD, SX127x_MODULATION_LORA, device));
        return;
    }
    // put into RX mode first to handle interrupt as soon as possible
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, device));
    ESP_LOGI(TAG, "cad detected\n");
}


void setup_loRa() {
    mod_spi_setup_rst(RST);
    ESP_LOGI(TAG, "sx127x was reset");

    //# Init SPI1 peripherals
    M_Spi_Conf spi_conf = {
        .host = 1,
        .mosi = 23,
        .miso = 19,
        .clk = 18,
        .cs = 5,
    };
    mod_spi_init(&spi_conf, 4E6);

    ESP_ERROR_CHECK(sx127x_create(spi_conf.spi_handle, &device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, SX127x_MODULATION_LORA, &device));
    ESP_ERROR_CHECK(sx127x_set_frequency(915000000, &device));  // 915MHz

    ESP_ERROR_CHECK(sx127x_lora_set_bandwidth(SX127x_BW_125000, &device));
    ESP_ERROR_CHECK(sx127x_lora_set_implicit_header(NULL, &device));
    ESP_ERROR_CHECK(sx127x_lora_set_modem_config_2(SX127x_SF_7, &device));
    ESP_ERROR_CHECK(sx127x_lora_set_syncword(18, &device));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, &device));
    ESP_ERROR_CHECK(sx127x_lora_reset_fifo(&device));

    // Setup DIO0 as input
    gpio_set_direction(DIO0, GPIO_MODE_INPUT);
    gpio_pulldown_en(DIO0);
    gpio_pullup_dis(DIO0);
}


//# mod_sx127_listen
void mod_sx127_listen() {
    setup_loRa();

    ESP_ERROR_CHECK(sx127x_rx_set_lna_boost_hf(true, &device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, SX127x_MODULATION_LORA, &device));
    ESP_ERROR_CHECK(sx127x_rx_set_lna_gain(SX127x_LNA_GAIN_G4, &device));

    sx127x_rx_set_callback(lora_rx_callback, &device);
    sx127x_lora_cad_set_callback(cad_callback, &device);

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, &device));

    while (1) {
        if (gpio_get_level(DIO0)) sx127x_handle_interrupt(&device);

        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent busy-waiting
    }
}


int messages_sent = 0;
int supported_power_levels[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20};
int supported_power_levels_count = sizeof(supported_power_levels) / sizeof(int);
int current_power_level = 0;
bool transmitting = false;

void start_transmission(sx127x *device) {
    if (messages_sent == 0) {
        uint8_t data[] = {0xCA, 0xFE};
        ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data, sizeof(data), device));
    } 
    else if (messages_sent == 1) {
        uint8_t data[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
        ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data, sizeof(data), device));
    }
    else if (messages_sent == 2) {
        uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a};
        ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data, sizeof(data), device));
    }
    else if (current_power_level < supported_power_levels_count) {
        uint8_t data[] = {0xCA, 0xFE};
        ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data, sizeof(data), device));
        ESP_ERROR_CHECK(sx127x_tx_set_pa_config(SX127x_PA_PIN_BOOST, supported_power_levels[current_power_level], device));
        current_power_level++;
    }

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, SX127x_MODULATION_LORA, device));
    transmitting = true;
    ESP_LOGI(TAG, "Started transmitting packet %d", messages_sent);
}

//# mod_sx127_send
void mod_sx127_send() {
    setup_loRa();

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, SX127x_MODULATION_LORA, &device));

    ESP_ERROR_CHECK(sx127x_tx_set_pa_config(SX127x_PA_PIN_BOOST, supported_power_levels[current_power_level], &device));
    sx127x_tx_header_t header = {
        .enable_crc = true,
        .coding_rate = SX127x_CR_4_5};
    ESP_ERROR_CHECK(sx127x_lora_tx_set_explicit_header(&header, &device));

    while(1) {
        if (!transmitting) {
            // Exit condition - after all test messages and power levels
            if (messages_sent >= 25 || current_power_level >= supported_power_levels_count) {
                ESP_LOGI(TAG, "All transmissions complete!");
                break; // Exit the while loop
            }

            start_transmission(&device);
        }
        else {
            if (gpio_get_level(DIO0)) {
                ESP_LOGI(TAG, "Packet %d transmitted", messages_sent);
                messages_sent++;
                transmitting = false;
                vTaskDelay(200 / portTICK_PERIOD_MS); // Short delay between transmissions
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS); // Yield to other tasks
    }
}