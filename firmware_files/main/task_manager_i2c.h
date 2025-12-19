#ifndef TASK_MANAGER_I2C_H
#define TASK_MANAGER_I2C_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "tasks_common.h"

#include "ina3221.h"
#include "ina219.h"

#define SENSOR_INA219
#define SENSOR_INA3221
#include "config.h"

/**
 * @brief Initialize the I2C master bus
 *
 * This function initializes the I2C master bus with default settings. 
 * It prevents re-initialization if the I2C bus is already initialized.
 *
 * @param None
 * @return ESP_OK if the I2C initialization is successful, otherwise an error code.
 */
esp_err_t task_manager_i2c_init(void);

/**
 * @brief Scan the I2C bus for connected devices
 *
 * This function scans the I2C bus for any devices and logs the detected device addresses.
 * It does not return anything but prints the device addresses found to the log.
 *
 * @param None
 * @return None
 */
void i2c_scan(void);

/**
 * @brief Manage the initialization of sensors and start data logging
 *
 * This function initializes the I2C bus, scans for connected devices, and 
 * creates tasks for INA219 and INA3221 sensors. It also notifies the sensor tasks 
 * to start data logging once the initialization is complete.
 *
 * @param None
 * @return None
 */
void sensor_task_manager(void);

/**
 * @brief Initialize and configure INA219 and INA3221 sensors
 *
 * This function configures and calibrates the INA219 sensor and initializes 
 * the INA3221 sensor. It sets options like channel activation, shunt resistors, 
 * averaging, and conversion times for INA3221. It also sets warning alerts for INA3221.
 *
 * @param[in] ina219 Pointer to an initialized `ina219_t` structure for INA219 sensor.
 * @param[in] ina3221 Pointer to an initialized `ina3221_t` structure for INA3221 sensor.
 * @return ESP_OK if both sensors are successfully initialized and configured, 
 *         otherwise an error code (`esp_err_t`).
 */
esp_err_t initialize_sensors(ina219_t *ina219, ina3221_t *ina3221);

// Initialize LED GPIO

void led_gpio_init(void) ;
#endif // TASK_MANAGER_I2C_H
