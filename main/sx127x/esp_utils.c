#include "esp_utils.h"
#include <esp_log.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

int total_packets_received = 0;
const UBaseType_t xArrayIndex = 0;
TaskHandle_t handle_interrupt;
static const char *TAG = "esp_utils";
void (*global_tx_callback)(sx127x *device);

void IRAM_ATTR handle_interrupt_fromisr(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveIndexedFromISR(handle_interrupt, xArrayIndex, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void setup_gpio_interrupts(gpio_num_t gpio, sx127x *device, gpio_int_type_t type) {
  ESP_ERROR_CHECK(gpio_set_direction(gpio, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_pulldown_en(gpio));
  ESP_ERROR_CHECK(gpio_pullup_dis(gpio));
  ESP_ERROR_CHECK(gpio_set_intr_type(gpio, type));
  ESP_ERROR_CHECK(gpio_isr_handler_add(gpio, handle_interrupt_fromisr, (void *) device));
}


//# handle_interrupt_tx_task
void handle_interrupt_tx_task(void *arg) {
  global_tx_callback((sx127x *) arg);
  while (1) {
    if (ulTaskNotifyTakeIndexed(xArrayIndex, pdTRUE, portMAX_DELAY) > 0) {
      sx127x_handle_interrupt((sx127x *) arg);
    }
  }
}

esp_err_t setup_tx_task(sx127x *device, void (*tx_callback)(sx127x *device)) {
  global_tx_callback = tx_callback;
  BaseType_t task_code = xTaskCreatePinnedToCore(handle_interrupt_tx_task, "handle interrupt", 8196, device, 2, &handle_interrupt, xPortGetCoreID());
  if (task_code != pdPASS) {
    ESP_LOGE(TAG, "can't create task %d", task_code);
    return ESP_FAIL;
  }
  return ESP_OK;
}
