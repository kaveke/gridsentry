/*
* Copyright 2015-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
* http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

/**
 * @file aws_iot_test_auto_reconnect.c
 * @brief Integration Test for automatic reconnect
 */

#include "aws_iot_test_integration_common.h"

static char ModifiedPathBuffer[PATH_MAX + 1];
char root_CA[PATH_MAX + 1];

bool terminate_yield_with_rc_thread = false;
IoT_Error_t yieldRC;
bool captureYieldReturnCode = false;
char * dummyLocation = "dummyLocation";
char * savedLocation;
/**
 * This function renames the rootCA.crt file to a temporary name to cause connect failure
 */
void aws_iot_mqtt_tests_block_tls_connect(AWS_IoT_Client *pClient) {
	savedLocation = pClient->networkStack.tlsConnectParams.pRootCALocation;
	pClient->networkStack.tlsConnectParams.pRootCALocation = dummyLocation;
}

/**
 * Always ensure this function is called after block_tls_connect
 */
void aws_iot_mqtt_tests_unblock_tls_connect(AWS_IoT_Client *pClient) {
	pClient->networkStack.tlsConnectParams.pRootCALocation = savedLocation;
}

void *aws_iot_mqtt_tests_yield_with_rc(void *ptr) {
	IoT_Error_t rc = SUCCESS;

	struct timeval start, end, result;
	static int cntr = 0;
	AWS_IoT_Client *pClient = (AWS_IoT_Client *) ptr;

	while(terminate_yield_with_rc_thread == false
		  && (NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)) {
		usleep(500000);
		ESP_LOGI(TAG, " Client state : %d ", aws_iot_mqtt_get_client_state(pClient));
		rc = aws_iot_mqtt_yield(pClient, 100);
		ESP_LOGI(TAG, "yield rc %d", rc);
		if(captureYieldReturnCode && SUCCESS != rc) {
			ESP_LOGI(TAG, "yield rc capture %d", rc);
			captureYieldReturnCode = false;
			yieldRC = rc;
		}
	}

	return NULL;
}

unsigned int disconnectedCounter = 0;

void aws_iot_mqtt_tests_disconnect_callback_handler(AWS_IoT_Client *pClient, void *param) {
	disconnectedCounter++;
}

int aws_iot_mqtt_tests_auto_reconnect() {
	pthread_t reconnectTester_thread, yield_thread;
	int yieldThreadReturn = 0;
	int test_result = 0;

	char certDirectory[15] = "../../certs";
	char CurrentWD[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char clientId[50];
	AWS_IoT_Client client;

	IoT_Error_t rc = SUCCESS;
	getcwd(CurrentWD, sizeof(CurrentWD));
	snESP_LOGI(TAG, root_CA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
	snESP_LOGI(TAG, clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
	snESP_LOGI(TAG, clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);
	srand((unsigned int) time(NULL));
	snESP_LOGI(TAG, clientId, 50, "%s_%d", INTEGRATION_TEST_CLIENT_ID, rand() % 10000);

	ESP_LOGI(TAG, " Root CA Path : %s\n clientCRT : %s\n clientKey : %s", root_CA, clientCRT, clientKey);
	IoT_Client_Init_Params initParams = IoT_Client_Init_Params_initializer;
	initParams.pHostURL = AWS_IOT_MQTT_HOST;
	initParams.port = AWS_IOT_MQTT_PORT;
	initParams.pRootCALocation = root_CA;
	initParams.pDeviceCertLocation = clientCRT;
	initParams.pDevicePrivateKeyLocation = clientKey;
	initParams.mqttCommandTimeout_ms = 20000;
	initParams.tlsHandshakeTimeout_ms = 5000;
	initParams.disconnectHandler = aws_iot_mqtt_tests_disconnect_callback_handler;
	initParams.enableAutoReconnect = false;
	aws_iot_mqtt_init(&client, &initParams);

	IoT_Client_Connect_Params connectParams;
	connectParams.keepAliveIntervalInSec = 5;
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = (char *) &clientId;
	connectParams.clientIDLen = strlen(clientId);
	connectParams.isWillMsgPresent = 0;
	connectParams.pUsername = NULL;
	connectParams.usernameLen = 0;
	connectParams.pPassword = NULL;
	connectParams.passwordLen = 0;

	rc = aws_iot_mqtt_connect(&client, &connectParams);
	if(rc != SUCCESS) {
		ESP_LOGE(TAG, "ERROR Connecting %d", rc);
		return -1;
	}

	yieldThreadReturn = pthread_create(&yield_thread, NULL, aws_iot_mqtt_tests_yield_with_rc, &client);

	/*
	 * Test disconnect handler
	 */
	ESP_LOGI(TAG, "1. Test Disconnect Handler");
	aws_iot_mqtt_tests_block_tls_connect(&client);
	iot_tls_disconnect(&(client.networkStack));
	sleep(connectParams.keepAliveIntervalInSec + 1);
	if(disconnectedCounter == 1) {
		ESP_LOGI(TAG, "Success invoking Disconnect Handler");
	} else {
		aws_iot_mqtt_tests_unblock_tls_connect(&client);
		ESP_LOGE(TAG, "Failure to invoke Disconnect Handler");
		return -1;
	}

	terminate_yield_with_rc_thread = true;
	pthread_join(yield_thread, NULL);
	aws_iot_mqtt_tests_unblock_tls_connect(&client);
	
	/*
	 * Manual Reconnect Test
	 */
	ESP_LOGI(TAG, "2. Test Manual Reconnect, Current Client state : %d ", aws_iot_mqtt_get_client_state(&client));
	rc = aws_iot_mqtt_attempt_reconnect(&client);
	if(rc != NETWORK_RECONNECTED) {
		ESP_LOGE(TAG, "ERROR reconnecting manually %d", rc);
		return -4;
	}
	terminate_yield_with_rc_thread = false;
	yieldThreadReturn = pthread_create(&yield_thread, NULL, aws_iot_mqtt_tests_yield_with_rc, &client);

	yieldRC = FAILURE;
	captureYieldReturnCode = true;

	// ensure atleast 1 cycle of yield is executed to get the yield status to SUCCESS
	sleep(1);
	if(!captureYieldReturnCode) {
		if(yieldRC == NETWORK_ATTEMPTING_RECONNECT) {
			ESP_LOGI(TAG, "Success reconnecting manually");
		} else {
			ESP_LOGE(TAG, "Failure to reconnect manually");
			return -3;
		}
	}
	terminate_yield_with_rc_thread = true;
	pthread_join(yield_thread, NULL);
	
	/*
	 * Auto Reconnect Test
	 */

	ESP_LOGI(TAG, "3. Test Auto_reconnect ");

	rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
	if(rc != SUCCESS) {
		ESP_LOGE(TAG, "Error: Failed to enable auto-reconnect %d ", rc);
	}

	yieldRC = FAILURE;
	captureYieldReturnCode = true;

	// Disconnect
	aws_iot_mqtt_tests_block_tls_connect(&client);
	iot_tls_disconnect(&(client.networkStack));

	terminate_yield_with_rc_thread = false;
	yieldThreadReturn = pthread_create(&yield_thread, NULL, aws_iot_mqtt_tests_yield_with_rc, &client);

	sleep(connectParams.keepAliveIntervalInSec + 1);
	if(!captureYieldReturnCode) {
		if(yieldRC == NETWORK_ATTEMPTING_RECONNECT) {
			ESP_LOGI(TAG, "Success attempting reconnect");
		} else {
			ESP_LOGE(TAG, "Failure to attempt to reconnect");
			return -6;
		}
	}
	if(disconnectedCounter == 2) {
		ESP_LOGI(TAG, "Success: disconnect handler invoked on enabling auto-reconnect");
	} else {
		ESP_LOGE(TAG, "Failure: disconnect handler not invoked on enabling auto-reconnect : %d", disconnectedCounter);
		return -7;
	}
	aws_iot_mqtt_tests_unblock_tls_connect(&client);
	sleep(connectParams.keepAliveIntervalInSec + 1);
	captureYieldReturnCode = true;
	sleep(connectParams.keepAliveIntervalInSec + 1);
	if(!captureYieldReturnCode) {
		if(yieldRC == SUCCESS) {
			ESP_LOGI(TAG, "Success attempting reconnect");
		} else {
			ESP_LOGE(TAG, "Failure to attempt to reconnect");
			return -6;
		}
	}
	
	terminate_yield_with_rc_thread = true;
	pthread_join(yield_thread, NULL);
	
	if(true == aws_iot_mqtt_is_client_connected(&client)) {
		ESP_LOGI(TAG, "Success: is Mqtt connected api");
	} else {
		ESP_LOGE(TAG, "Failure: is Mqtt Connected api");
		return -7;
	}

	rc = aws_iot_mqtt_disconnect(&client);
	return rc;
}
