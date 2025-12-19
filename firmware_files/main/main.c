/**
 * Application entry point
 */

#include "esp_log.h"
#include "nvs_flash.h"

#include "aws_iot.h"
#include "task_manager_i2c.h"
#include "sntp_time_sync.h"
#include "wifi_app.h"
#include "wifi_reset_button.h"
#include "monitor_health.h"


static const char TAG[] = "main";

void wifi_application_connected_events(void)
{
    // Trigger time synchronization and initialize AWS IoT once WiFi is connected
    ESP_LOGI(TAG, "Wifi Application Connected!");
    sntp_time_sync_task_start();  // Sync time using SNTP protocol
    
}

void app_main(void)
{
    // Monitor system health by tracking stack memory and other metrics
    ESP_LOGI(HEALTH_MONITOR_TAG, "Starting system monitoring");
    check_reset_reason();  // Check system reset reason to handle unexpected resets
    xTaskCreate(monitor_system_health, "monitor_system_health", 4096, NULL, 1, NULL);  // Start system health monitoring task

    // Initialize NVS (Non-Volatile Storage) to store system state across reboots
    esp_err_t ret = nvs_flash_init();  // Initialize NVS flash memory
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // If the NVS partition is full or the version is outdated, erase it and reinitialize
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();  // Re-initialize NVS
    }
    ESP_ERROR_CHECK(ret);  // Ensure that NVS initialization is successful

    // Optional sensor initialization for verification (if required for debugging)
    //ESP_LOGI(TAG, "Initializing and calibrating sensors...");
    //sensor_task_manager();  // Call to initialize sensors if necessary

    // Start WiFi connection process
    wifi_app_start();  // Initialize WiFi and start connection

    // Set the WiFi connected event callback function
    wifi_app_set_callback(&wifi_application_connected_events);
}
