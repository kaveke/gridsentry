/*
* Copyright 2015-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
 * @file aws_iot_test_integration_runner.c
 * @brief Integration Test runner
 */

#include "aws_iot_test_integration_common.h"

int main() {
	int rc = 0;

	ESP_LOGI(TAG, "\n");
	ESP_LOGI(TAG, "*************************************************************************************************");
	ESP_LOGI(TAG, "* Starting TEST 1 MQTT Version 3.1.1 Basic Subscribe QoS 1 Publish QoS 1 with Single Client     *");
	ESP_LOGI(TAG, "*************************************************************************************************");
	rc = aws_iot_mqtt_tests_basic_connectivity();
	if(0 != rc) {
		ESP_LOGI(TAG, "\n********************************************************************************************************");
		ESP_LOGE(TAG, "* TEST 1 MQTT Version 3.1.1 Basic Subscribe QoS 1 Publish QoS 1 with Single Client FAILED! RC : %4d   *", rc);
		ESP_LOGI(TAG, "********************************************************************************************************");
		return 1;
	}
	ESP_LOGI(TAG, "\n*************************************************************************************************");
	ESP_LOGI(TAG, "* TEST 1 MQTT Version 3.1.1 Basic Subscribe QoS 1 Publish QoS 1 with Single Client SUCCESS!!    *");
	ESP_LOGI(TAG, "*************************************************************************************************");

	ESP_LOGI(TAG, "\n");
	ESP_LOGI(TAG, "************************************************************************************************************");
	ESP_LOGI(TAG, "* Starting TEST 2 MQTT Version 3.1.1 Multithreaded Subscribe QoS 1 Publish QoS 1 with Multiple Clients     *");
	ESP_LOGI(TAG, "************************************************************************************************************");
	rc = aws_iot_mqtt_tests_multiple_clients();
	if(0 != rc) {
		ESP_LOGI(TAG, "\n*******************************************************************************************************************");
		ESP_LOGE(TAG, "* TEST 2 MQTT Version 3.1.1 Multithreaded Subscribe QoS 1 Publish QoS 1 with Multiple Clients FAILED! RC : %4d   *", rc);
		ESP_LOGI(TAG, "*******************************************************************************************************************");
		return 1;
	}
	ESP_LOGI(TAG, "\n*************************************************************************************************************");
	ESP_LOGI(TAG, "* TEST 2 MQTT Version 3.1.1 Multithreaded Subscribe QoS 1 Publish QoS 1 with Multiple Clients SUCCESS!!     *");
	ESP_LOGI(TAG, "*************************************************************************************************************");

	ESP_LOGI(TAG, "\n");
	ESP_LOGI(TAG, "*********************************************************");
	ESP_LOGI(TAG, "* Starting TEST 3 MQTT Version 3.1.1 Auto Reconnect     *");
	ESP_LOGI(TAG, "*********************************************************");
	rc = aws_iot_mqtt_tests_auto_reconnect();
	if(0 != rc) {
		ESP_LOGI(TAG, "\n***************************************************************");
		ESP_LOGE(TAG, "* TEST 3 MQTT Version 3.1.1 Auto Reconnect FAILED! RC : %4d  *", rc);
		ESP_LOGI(TAG, "***************************************************************");
		return 1;
	}
	ESP_LOGI(TAG, "\n**********************************************************");
	ESP_LOGI(TAG, "* TEST 3 MQTT Version 3.1.1 Auto Reconnect SUCCESS!!     *");
	ESP_LOGI(TAG, "**********************************************************");

#ifndef DISABLE_IOT_JOBS
#ifndef DISABLE_IOT_JOBS_INTERFACE
	ESP_LOGI(TAG, "\n");
	ESP_LOGI(TAG, "*************************************************************************************************");
	ESP_LOGI(TAG, "* Starting TEST 4 Jobs API Test                                                                 *");
	ESP_LOGI(TAG, "*************************************************************************************************");
	rc = aws_iot_jobs_basic_test();
	if(0 != rc) {
		ESP_LOGI(TAG, "\n********************************************************************************************************");
		ESP_LOGE(TAG, "* TEST 4 Jobs API Test  FAILED! RC : %4d                                                              *", rc);
		ESP_LOGI(TAG, "********************************************************************************************************");
		return 1;
	}
	ESP_LOGI(TAG, "\n*************************************************************************************************");
	ESP_LOGI(TAG, "* TEST 4 Jobs API Test SUCCESS!!                                                                *");
	ESP_LOGI(TAG, "*************************************************************************************************");
#endif
#endif

	return 0;
}
