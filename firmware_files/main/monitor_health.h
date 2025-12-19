#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static char *HEALTH_MONITOR_TAG = "SYSTEM_MONITOR";

void check_reset_reason() {
    esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI(HEALTH_MONITOR_TAG, "Last reset reason: %d", reason);
    switch (reason) {
        case ESP_RST_POWERON: ESP_LOGI(HEALTH_MONITOR_TAG, "Power-on reset"); break;
        case ESP_RST_EXT: ESP_LOGI(HEALTH_MONITOR_TAG, "External reset"); break;
        case ESP_RST_SW: ESP_LOGI(HEALTH_MONITOR_TAG, "Software reset"); break;
        case ESP_RST_PANIC: ESP_LOGI(HEALTH_MONITOR_TAG, "Exception/panic"); break;
        case ESP_RST_INT_WDT: ESP_LOGI(HEALTH_MONITOR_TAG, "Interrupt watchdog"); break;
        case ESP_RST_TASK_WDT: ESP_LOGI(HEALTH_MONITOR_TAG, "Task watchdog"); break;
        case ESP_RST_BROWNOUT: ESP_LOGI(HEALTH_MONITOR_TAG, "Brownout"); break;
        case ESP_RST_UNKNOWN: ESP_LOGI(HEALTH_MONITOR_TAG, "Other reason"); break;
        default: ESP_LOGI(HEALTH_MONITOR_TAG, "Unknown reason"); break;
    }
}

void monitor_system_health(void *pvParameters) {
    while (1) {
        ESP_LOGI(HEALTH_MONITOR_TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(30000)); // Log every 30 seconds
    }
}