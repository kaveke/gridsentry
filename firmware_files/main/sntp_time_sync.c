#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"

#include "tasks_common.h"
#include "aws_iot.h"
#include "http_server.h"
#include "sntp_time_sync.h"
#include "wifi_app.h"

static const char TAG[] = "sntp_time_sync";

// SNTP operating mode set status
static bool sntp_op_mode_set = false;

/**
 * Initialise SNTP service using SNTP_OPMODE_POLL mode
 */
static void sntp_time_sync_init_sntp(void)
{
    ESP_LOGI(TAG, "INitialising the SNTP service");

    if(!sntp_op_mode_set)
    {
        // Set the operating mode
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_op_mode_set = true;
    }

    sntp_setservername(0, "pool.ntp.org");

    // Initialise the servers
    sntp_init();

    // Let the http_server know service is initialised
    http_server_monitor_send_message(HTTP_MSG_TIME_SERVICE_INITIALISED);
}

/**
 * Gets the current time and if the current time is not up to date,
 * the sntp_time_sync_init_sntp function is called
 */
void sntp_time_sync_obtain_time(void)
{
    time_t now = 0;
    struct tm time_info = {0};

    time(&now);
    localtime_r(&now, &time_info);

    if (time_info.tm_year < (2025 - 1900)) {
        ESP_LOGI(TAG, "Time not set. Initialising SNTP...");

        sntp_time_sync_init_sntp();

        // Wait until time is synced
        int retry = 0;
        const int retry_count = 10;

        while (time_info.tm_year < (2025 - 1900) && ++retry < retry_count) {
            ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        // Set local timezone only after time is synced
        ESP_LOGI(TAG, "System time synchronized successfully.");
        // CONFIGURATION PATCH: Use Kconfig value for timezone
        setenv("TZ", CONFIG_APP_TIMEZONE, 1);
        tzset();
    }
}

/**
 * The SNTP time synchronisation task
 * @return arg pvParam
 */
static void sntp_time_sync(void *pvParam) 
{
    int retry = 0;
    const int retry_count = 5;

    ESP_LOGI(TAG, "Waiting for time synchronization...");

    sntp_time_sync_obtain_time();  // initialize SNTP

    while (!sntp_time_synced() && ++retry < retry_count) {
        ESP_LOGW(TAG, "Time not synchronized yet... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (sntp_time_synced()) {
        ESP_LOGI(TAG, "Time synchronized!");
        aws_iot_start();  // Start AWS IoT service and initiate sensor readings through AWS
    } else {
        ESP_LOGE(TAG, "Failed to synchronize time after retries");
    }

    vTaskDelete(NULL);
}


bool sntp_time_synced() {
    time_t now;
    struct tm time_info;

    time(&now);
    localtime_r(&now, &time_info);

    // Return true if the year is valid (indicating time is synced)
    return (time_info.tm_year >= (2025 - 1900));
}


char* sntp_time_sync_get_time(void)
{
    static char time_buffer[100] = {0};

    time_t now = 0;
    struct tm time_info = {0};

    time(&now);
    localtime_r(&now, &time_info);

    if (time_info.tm_year < (2025-1900))
    {
        ESP_LOGI(TAG, "Time is not set yet");
    }
    else
    {
        strftime(time_buffer, sizeof(time_buffer), "%d.%m.%y %H:%M:%S", &time_info);
        ESP_LOGI(TAG, "Current time info: %s", time_buffer);
    }

    return time_buffer;
}

void sntp_time_sync_task_start(void)
{
    xTaskCreatePinnedToCore(&sntp_time_sync, "sntp_time_sync", SNTP_TIME_SYNC_TASK_STACK_SIZE, NULL, SNTP_TIME_SYNC_TASK_PRIORITY, NULL, SNTP_TIME_SYNC_TASK_CORE_ID);
}