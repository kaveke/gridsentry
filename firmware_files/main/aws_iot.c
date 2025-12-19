/*
 * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * Additions Copyright 2016 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
/**
 * @file subscribe_publish_sample.c
 * @brief simple MQTT publish and subscribe on the same topic
 *
 * This example takes the parameters from the build configuration and establishes a connection to the AWS IoT MQTT Platform.
 * It subscribes and publishes to the same topic - "test_topic/esp32"
 *
 * Some setup is required. See example README for details.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "cJSON.h"

#include "aws_iot.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "tasks_common.h"
#include "wifi_app.h"
#include "sntp_time_sync.h"

#include "task_manager_i2c.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

static const char *TAG = "aws_iot";

// AWS IoT task handle
static TaskHandle_t task_aws_iot = NULL;

/**
 * CA Root certificate, device ("Thing") certificate and device ("Thing") key.
 * "Embedded Certs" are loaded from files in "certs/" and embedded into the app binary.
 */
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");

extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");

extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");


/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;

void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData) {
    ESP_LOGI(TAG, "Subscribe callback received on topic: %.*s", topicNameLen, topicName);
    ESP_LOGD(TAG, "Payload: %.*s", (int) params->payloadLen, (char *)params->payload);
}

void iot_prediction_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                     IoT_Publish_Message_Params *params, void *pData) {
    ESP_LOGI(TAG, "Received prediction: %.*s", params->payloadLen, (char *)params->payload);

    char *payload_str = strndup((char *)params->payload, params->payloadLen);
    if (!payload_str) return;

    cJSON *root = cJSON_Parse(payload_str);
    free(payload_str);

    if (!root) {
        ESP_LOGE(TAG, "Failed to parse prediction JSON");
        return;
    }

    cJSON *prediction = cJSON_GetObjectItem(root, "prediction");
    if (prediction && cJSON_IsArray(prediction)) {
        cJSON *label = cJSON_GetArrayItem(prediction, 0);
        if (label && cJSON_IsString(label)) {
            ESP_LOGI(TAG, "Parsed prediction value: %s", label->valuestring);

            if (strcmp(label->valuestring, "c1") == 0 ||
                strcmp(label->valuestring, "c2") == 0 ||
                strcmp(label->valuestring, "c3") == 0) {
                
                ESP_LOGW(TAG, "Theft detected on %s", label->valuestring);

                // Blink LED rapidly for ~2 seconds (20 toggles at 100ms)
                for (int i = 0; i < 10; ++i) {
                    gpio_set_level(LED_GPIO, 1);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    gpio_set_level(LED_GPIO, 0);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
            } else {
                ESP_LOGI(TAG, "No theft detected (normal)");
            }
        }
    }

    cJSON_Delete(root);
}


void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
    ESP_LOGW(TAG, "MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    if(NULL == pClient) {
        return;
    }

    if(aws_iot_is_autoreconnect_enabled(pClient)) {
        ESP_LOGI(TAG, "Auto Reconnect is enabled, Reconnecting attempt will start now");
    } else {
        ESP_LOGW(TAG, "Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(pClient);
        if(NETWORK_RECONNECTED == rc) {
            ESP_LOGW(TAG, "Manual Reconnect Successful");
        } else {
            ESP_LOGW(TAG, "Manual Reconnect Failed - %d", rc);
        }
    }
}

void aws_iot_task(void *param) {
    char cPayload[1024]; // Large size to accomodate the JSON payload

    IoT_Error_t rc = FAILURE;

    AWS_IoT_Client client;
    IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
    IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

    IoT_Publish_Message_Params paramsQOS0;
    IoT_Publish_Message_Params paramsQOS1; //> Uncomment if needed, explanation in the block at the bottom

    esp_err_t err;
    ina219_t *ina219;
    ina3221_t *ina3221;
    float bus_voltage = 0.0, shunt_voltage = 0.0, shunt_current = 0.0, current = 0.0;

    time_t now;
    char time_str[32];
    struct tm timeinfo;

    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    // Wait for SNTP synchronization
    ESP_LOGI(TAG, "Waiting for time synchronization...");
    while (!sntp_time_synced()) {
        ESP_LOGW(TAG, "Time not synchronized yet...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Time synchronized!");

    mqttInitParams.enableAutoReconnect = false; // We enable this later below
    mqttInitParams.pHostURL = HostAddress;
    mqttInitParams.port = port;

    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)private_pem_key_start;

    mqttInitParams.mqttCommandTimeout_ms = 20000;
    mqttInitParams.tlsHandshakeTimeout_ms = 5000;
    mqttInitParams.isSSLHostnameVerify = true;
    mqttInitParams.disconnectHandler = disconnectCallbackHandler;
    mqttInitParams.disconnectHandlerData = NULL;

    rc = aws_iot_mqtt_init(&client, &mqttInitParams);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "aws_iot_mqtt_init returned error : %d ", rc);
        abort();
    }

    connectParams.keepAliveIntervalInSec = 10;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    /* Client ID is set in aws_iot.h and AKA your Thing's Name in AWS IoT */
    connectParams.pClientID = CONFIG_AWS_EXAMPLE_CLIENT_ID;
    connectParams.clientIDLen = (uint16_t) strlen(CONFIG_AWS_EXAMPLE_CLIENT_ID);
    connectParams.isWillMsgPresent = false;

    ESP_LOGI(TAG, "Connecting to AWS...");
    do {
        rc = aws_iot_mqtt_connect(&client, &connectParams);
        if(SUCCESS != rc) {
            ESP_LOGE(TAG, "Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while(SUCCESS != rc);

    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Unable to set Auto Reconnect to true - %d", rc);
        abort();
    }

    const char *TOPIC = "smartmeter/data";
    const int TOPIC_LEN = strlen(TOPIC);

    ESP_LOGI(TAG, "Subscribing...");
    rc = aws_iot_mqtt_subscribe(&client, TOPIC, TOPIC_LEN, QOS0, iot_subscribe_callback_handler, NULL);
    if(SUCCESS != rc) {
        ESP_LOGE(TAG, "Error subscribing : %d ", rc);
        abort();
    }

    const char *PREDICTION_TOPIC = "smartmeter/prediction";
    const int PREDICTION_TOPIC_LEN = strlen(PREDICTION_TOPIC);

    ESP_LOGI(TAG, "Subscribing to prediction topic...");
    rc = aws_iot_mqtt_subscribe(&client, PREDICTION_TOPIC, PREDICTION_TOPIC_LEN, QOS1, iot_prediction_callback_handler, NULL);
    if (SUCCESS != rc) {
        ESP_LOGE(TAG, "Error subscribing to prediction topic: %d", rc);
        abort();
    }

    // Allocate memory for INA219 and INA3221 sensors
    ina219 = (ina219_t *)malloc(sizeof(ina219_t));
    if (ina219 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for INA219");
        abort();
    }

    ina3221 = (ina3221_t *)malloc(sizeof(ina3221_t));
    if (ina3221 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for INA3221");
        free((void*)ina219);
        abort();
    }

    // Initialize sensors
    err = initialize_sensors(ina219, ina3221);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensors: %s", esp_err_to_name(err));
        free((void*)ina219);
        free((void*)ina3221);
        abort();
    }

    paramsQOS0.qos = QOS0;
    paramsQOS0.payload = (void *) cPayload;
    paramsQOS0.isRetained = 0;

    paramsQOS1.qos = QOS1;
    paramsQOS1.payload = (void *) cPayload;
    paramsQOS1.isRetained = 0;

    while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)) {

        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&client, 100);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }

        ESP_LOGI(TAG, "Stack remaining for task '%s' is %d bytes", pcTaskGetName(NULL), uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(3000 / portTICK_PERIOD_MS);

        while(1) {
            // --- INA219 Feeder Measurements ---
            float feeder_line_voltage = 0.0;  // mV
            float feeder_shunt_voltage = 0.0; // mV
            float feeder_current = 0.0;       // mA

            // --- INA219 Feeder Measurements ---
            err = ina219_get_bus_voltage(ina219, &bus_voltage);
            if (err == ESP_OK) feeder_line_voltage = bus_voltage ;  // Line voltage in V

            err = ina219_get_shunt_voltage(ina219, &shunt_voltage);
            if (err == ESP_OK) feeder_shunt_voltage = shunt_voltage * 1000;  // Shunt voltage in mV

            err = ina219_get_current(ina219, &current);
            if (err == ESP_OK) feeder_current = current * 1000;  // Current in mA


            // --- INA3221 Transformer Measurements ---
            float transformer_shunt_voltage[3] = {0.0, 0.0, 0.0};
            float transformer_current[3] = {0.0, 0.0, 0.0};

            for (uint8_t i = 0; i < INA3221_BUS_NUMBER; i++) {
                err = ina3221_get_shunt_value(ina3221, i, &shunt_voltage, &shunt_current);
                if (err == ESP_OK) {
                    transformer_shunt_voltage[i] = shunt_voltage; //Voltage - mV
                    transformer_current[i] = shunt_current; // Current - mA
                }
            }
            
            time(&now);
            localtime_r(&now, &timeinfo);
            strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S+03:00", &timeinfo);  // ISO 8601 format

            // --- Construct JSON Payload ---
            // Compose JSON payload with sensor readings
            int required_size = snprintf(NULL, 0,
                "{\"timestamp\":\"%s\","
                "\"feeder\":{\"line_voltage\":%.2f,\"shunt_voltage\":%.3f,\"current\":%.3f},"
                "\"transformer1\":{\"shunt_voltage\":%.2f,\"current\":%.3f},"
                "\"transformer2\":{\"shunt_voltage\":%.2f,\"current\":%.3f},"
                "\"transformer3\":{\"shunt_voltage\":%.2f,\"current\":%.3f}}",
                time_str,
                feeder_line_voltage, feeder_shunt_voltage, feeder_current,
                transformer_shunt_voltage[0], transformer_current[0],
                transformer_shunt_voltage[1], transformer_current[1],
                transformer_shunt_voltage[2], transformer_current[2]);

            if (required_size >= sizeof(cPayload)) {
                ESP_LOGE(TAG, "Error: Payload size %d exceeds buffer size %zu", required_size, sizeof(cPayload));
            } else {
                snprintf(cPayload, sizeof(cPayload),
                    "{\"timestamp\":\"%s\","
                    "\"feeder\":{\"line_voltage\":%.2f,\"shunt_voltage\":%.3f,\"current\":%.3f},"
                    "\"transformer1\":{\"shunt_voltage\":%.2f,\"current\":%.3f},"
                    "\"transformer2\":{\"shunt_voltage\":%.2f,\"current\":%.3f},"
                    "\"transformer3\":{\"shunt_voltage\":%.2f,\"current\":%.3f}}",
                    time_str,
                    feeder_line_voltage, feeder_shunt_voltage, feeder_current,
                    transformer_shunt_voltage[0], transformer_current[0],
                    transformer_shunt_voltage[1], transformer_current[1],
                    transformer_shunt_voltage[2], transformer_current[2]);
            }

            // --- Publish Payload to AWS IoT ---
            paramsQOS0.payload = (void *)cPayload;
            paramsQOS0.payloadLen = strlen(cPayload);
            rc = aws_iot_mqtt_publish(&client, TOPIC, TOPIC_LEN, &paramsQOS0);
            if (rc != SUCCESS) {
                ESP_LOGE(TAG, "Error publishing QOS0: %d", rc);
            }

            /**
             *  --- QoS 1 Publish Block ---
             * 
             *  This section publishes the same payload using QoS 1, which ensures delivery
             *  by requiring an acknowledgment from AWS IoT Core.
             * 
             *  Use this only if reliable message delivery is necessary, such as for commands
             *  or critical data points.
             * 
             *  Note: QoS 1 introduces more overhead and can delay publishing if acknowledgments
             *  are not received.
             * 
             *  Uncomment this block if you need guaranteed delivery.
             *  Example use case: device control commands or one-time alerts.
             */
            
            paramsQOS1.payload = (void *)cPayload;
            paramsQOS1.payloadLen = strlen(cPayload);
            rc = aws_iot_mqtt_publish(&client, TOPIC, TOPIC_LEN, &paramsQOS1);
            if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
                ESP_LOGW(TAG, "QOS1 publish ack not received.");
                rc = SUCCESS;
            }
           
            vTaskDelay(pdMS_TO_TICKS(5000));  // Adjust the delay as needed
        }

    }

    ESP_LOGE(TAG, "An error occurred in the main loop.");
    abort();
}

void aws_iot_start(void)
{
	if (task_aws_iot == NULL)
	{
		xTaskCreatePinnedToCore(&aws_iot_task, "aws_iot_task", AWS_IOT_TASK_STACK_SIZE, NULL, AWS_IOT_TASK_PRIORITY, &task_aws_iot, AWS_IOT_TASK_CORE_ID);
	}
}
