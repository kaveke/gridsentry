/*
 * Copyright (c) 2019 Ruslan V. Uss <unclerus@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of itscontributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file ina219.c
 *
 * ESP-IDF driver for INA219/INA220 Zerø-Drift, Bidirectional
 * Current/Power Monitor
 *
 * Copyright (c) 2019 Ruslan V. Uss <unclerus@gmail.com>
 *
 * BSD Licensed as described in the file LICENSE
 */
#include <stdio.h>
#include <esp_log.h>
#include <math.h>
#include <esp_idf_lib_helpers.h>
#include <string.h>
#include "ina219.h"
#include "task_manager_i2c.h"

#define SENSOR_INA219
#include "config.h"

#define I2C_FREQ_HZ 1000000 // Max 1 MHz for esp-idf, but supports up to 2.56 MHz
#define I2C_TIMEOUT_MS 1000
#define I2C_NO_STOP 0
#define I2C_STOP    1

static const char *INA219_TAG = "INA219";

#define REG_CONFIG      0
#define REG_SHUNT_U     1
#define REG_BUS_U       2
#define REG_POWER       3
#define REG_CURRENT     4
#define REG_CALIBRATION 5

#define BIT_RST   15
#define BIT_BRNG  13
#define BIT_PG0   11
#define BIT_BADC0 7
#define BIT_SADC0 3
#define BIT_MODE  0

#define MASK_PG   (3 << BIT_PG0)
#define MASK_BADC (0xf << BIT_BADC0)
#define MASK_SADC (0xf << BIT_SADC0)
#define MASK_MODE (7 << BIT_MODE)
#define MASK_BRNG (1 << BIT_BRNG)

#define DEF_CONFIG 0x399f

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

// Improvement 6: Added error handling for mutex operations
#define CHECK_MUTEX_TAKE(x) do { \
    if (xSemaphoreTake((x), portMAX_DELAY) != pdTRUE) { \
        ESP_LOGE(INA219_TAG, "Failed to acquire mutex"); \
        vTaskDelete(NULL); \
        return; \
    } \
} while (0)

#define CHECK_MUTEX_GIVE(x) do { \
    if (xSemaphoreGive((x)) != pdTRUE) { \
        ESP_LOGE(INA219_TAG, "Failed to release mutex"); \
        vTaskDelete(NULL); \
        return; \
    } \
} while (0)

i2c_master_bus_handle_t i2c_bus_handle = NULL;
i2c_master_dev_handle_t i2c_dev_handle = NULL;

static const float u_shunt_max[] = {
    [INA219_GAIN_1]     = 0.04,
    [INA219_GAIN_0_5]   = 0.08,
    [INA219_GAIN_0_25]  = 0.16,
    [INA219_GAIN_0_125] = 0.32,
};

static esp_err_t ina219_read_register(ina219_t *dev, uint8_t reg, uint16_t *data) {
    if (dev == NULL || dev->i2c_dev_handle == NULL) {
        ESP_LOGE(INA219_TAG, "Invalid device handle");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buf[2] = {0};
    esp_err_t err = i2c_master_transmit_receive(dev->i2c_dev_handle, &reg, 1, buf, 2, I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(INA219_TAG, "Failed to read register 0x%02X: %s", reg, esp_err_to_name(err));
        return err;
    }
    *data = (buf[0] << 8) | buf[1];
    return ESP_OK;
}

static esp_err_t ina219_write_register(ina219_t *dev, uint8_t reg, uint16_t data)
{
    if (dev == NULL || dev->i2c_dev_handle == NULL) {
        ESP_LOGE(INA219_TAG, "Invalid device handle");
        return ESP_ERR_INVALID_ARG;
    }

    // Swap bytes for big-endian format
    uint8_t buf[3] = {reg, data >> 8, data & 0xFF};
    
    esp_err_t err = i2c_master_transmit(dev->i2c_dev_handle, buf, sizeof(buf), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(INA219_TAG, "Failed to write register 0x%02X: %s", reg, esp_err_to_name(err));
    }

    return err;
}


static esp_err_t read_conf_bits(ina219_t *dev, uint16_t mask, uint8_t bit, uint16_t *res)
{
    uint16_t raw;
    CHECK(ina219_read_register(dev, REG_CONFIG, &raw));

    *res = (raw & mask) >> bit;

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t ina219_init_desc(ina219_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio) {
    CHECK_ARG(dev);

    if (addr < INA219_ADDR_GND_GND || addr > INA219_ADDR_SCL_SCL) {
        ESP_LOGE(INA219_TAG, "Invalid I2C address");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_master_bus_config_t i2c_bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = port,
        .scl_io_num = scl_gpio,
        .sda_io_num = sda_gpio,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &dev->i2c_bus_handle));

    i2c_device_config_t i2c_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(dev->i2c_bus_handle, &i2c_dev_cfg, &dev->i2c_dev_handle));

    return ESP_OK;
}

esp_err_t ina219_free_desc(ina219_t *dev) {
    CHECK_ARG(dev);

    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev->i2c_dev_handle));
    ESP_ERROR_CHECK(i2c_del_master_bus(dev->i2c_bus_handle));

    return ESP_OK;
}


esp_err_t ina219_init(i2c_master_bus_handle_t i2c_bus, ina219_t *ina219, uint8_t i2c_address) {
    if (!ina219) {
        ESP_LOGE(INA219_TAG, "INA219 handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (i2c_bus == NULL) {
        ESP_LOGE(INA219_TAG, "I2C bus handle is NULL. Did you call task_manager_i2c_init()?");
        return ESP_ERR_INVALID_STATE;
    }

    memset(ina219, 0, sizeof(ina219_t));
    ina219->i2c_addr = i2c_address;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_address,
        .scl_speed_hz = 100000,
        .scl_wait_us = I2C_TIMEOUT_MS
    };

    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &ina219->i2c_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(INA219_TAG, "Failed to add INA219 device to I2C bus, error: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(INA219_TAG, "INA219 initialized at address 0x%02X", i2c_address);
    return ESP_OK;
}

esp_err_t ina219_reset(ina219_t *dev)
{
    CHECK_ARG(dev);
    CHECK(ina219_write_register(dev, REG_CONFIG, 1 << BIT_RST));

    dev->config = DEF_CONFIG;

    ESP_LOGD(INA219_TAG, "Device reset");

    return ESP_OK;
}

 
esp_err_t ina219_configure(ina219_t *dev, ina219_bus_voltage_range_t u_range,
        ina219_gain_t gain, ina219_resolution_t u_res,
        ina219_resolution_t i_res, ina219_mode_t mode)
{
    CHECK_ARG(dev);
    CHECK_ARG(u_range <= INA219_BUS_RANGE_32V);
    CHECK_ARG(gain <= INA219_GAIN_0_125);
    CHECK_ARG(u_res <= INA219_RES_12BIT_128S);
    CHECK_ARG(i_res <= INA219_RES_12BIT_128S);
    CHECK_ARG(mode <= INA219_MODE_CONT_SHUNT_BUS);

    dev->config = (u_range << BIT_BRNG) |
                  (gain << BIT_PG0) |
                  (u_res << BIT_BADC0) |
                  (i_res << BIT_SADC0) |
                  (mode << BIT_MODE);

    ESP_LOGI(INA219_TAG, "Configuration successful! Config setting: 0x%04x", dev->config);

    return ina219_write_register(dev, REG_CONFIG, dev->config);
}

esp_err_t ina219_get_bus_voltage_range(ina219_t *dev, ina219_bus_voltage_range_t *range)
{
    CHECK_ARG(dev && range);
    *range = 0;
    return read_conf_bits(dev, MASK_BRNG, BIT_BRNG, (uint16_t *)range);
}

esp_err_t ina219_get_gain(ina219_t *dev, ina219_gain_t *gain)
{
    CHECK_ARG(dev && gain);
    *gain = 0;
    return read_conf_bits(dev, MASK_PG, BIT_PG0, (uint16_t *)gain);
}

esp_err_t ina219_get_bus_voltage_resolution(ina219_t *dev, ina219_resolution_t *res)
{
    CHECK_ARG(dev && res);
    *res = 0;
    return read_conf_bits(dev, MASK_BADC, BIT_BADC0, (uint16_t *)res);
}

esp_err_t ina219_get_shunt_voltage_resolution(ina219_t *dev, ina219_resolution_t *res)
{
    CHECK_ARG(dev && res);
    *res = 0;
    return read_conf_bits(dev, MASK_SADC, BIT_SADC0, (uint16_t *)res);
}

esp_err_t ina219_get_mode(ina219_t *dev, ina219_mode_t *mode)
{
    CHECK_ARG(dev && mode);
    *mode = 0;
    return read_conf_bits(dev, MASK_MODE, BIT_MODE, (uint16_t *)mode);
}

esp_err_t ina219_calibrate(ina219_t *dev, float r_shunt, float max_current)
{
    CHECK_ARG(dev);

    ina219_gain_t gain;
    CHECK(ina219_get_gain(dev, &gain));

    // Calculate current LSB - max current determines resolution if provided, else use shunt max
    if (max_current > 0) {
        dev->i_lsb = max_current / 32767.0;
    } else {
        dev->i_lsb = u_shunt_max[gain] / r_shunt / 32767.0;
    }

    // Round i_lsb to nearest 0.0001 for accuracy
    dev->i_lsb = ceil(dev->i_lsb * 10000) / 10000.0;

    // Calculate power LSB
    dev->p_lsb = dev->i_lsb * 20;

    // Calculate calibration register value
    uint16_t cal = (uint16_t)(0.04096 / (dev->i_lsb * r_shunt));

    ESP_LOGI(INA219_TAG, "Calibration: R_shunt=%.04f Ohm, I_LSB=%.06f A/bit, P_LSB=%.06f W/bit, Cal=0x%04x",
             r_shunt, dev->i_lsb, dev->p_lsb, cal);

    // Write calibration value to the device
    esp_err_t err = ina219_write_register(dev, REG_CALIBRATION, cal);
    if (err != ESP_OK) {
        ESP_LOGE(INA219_TAG, "Failed to write calibration: %s", esp_err_to_name(err));
        return err;
    }

    // Optional: Verify calibration by reading it back
    uint16_t cal_read;
    err = ina219_read_register(dev, REG_CALIBRATION, &cal_read);
    if (err != ESP_OK || cal_read != cal) {
        ESP_LOGW(INA219_TAG, "Calibration verification failed (expected: 0x%04x, read: 0x%04x)", cal, cal_read);
    } else {
        ESP_LOGI(INA219_TAG, "Calibration successful (0x%04x)", cal_read);
    }

    return ESP_OK;
}

esp_err_t ina219_trigger(ina219_t *dev)
{
    CHECK_ARG(dev);

    uint16_t mode = (dev->config & MASK_MODE) >> BIT_MODE;
    if (mode < INA219_MODE_TRIG_SHUNT || mode > INA219_MODE_TRIG_SHUNT_BUS)
    {
        ESP_LOGE(INA219_TAG, "Could not trigger conversion in this mode: %d", mode);
        return ESP_ERR_INVALID_STATE;
    }

    return ina219_write_register(dev, REG_CONFIG, dev->config);
}

esp_err_t ina219_get_bus_voltage(ina219_t *dev, float *voltage) {
    CHECK_ARG(dev && voltage);

    uint16_t raw;
    CHECK(ina219_read_register(dev, REG_BUS_U, &raw));

    *voltage = (raw >> 3) * 0.004;

    return ESP_OK;
}

esp_err_t ina219_get_shunt_voltage(ina219_t *dev, float *voltage) {
    CHECK_ARG(dev && voltage);

    int16_t raw;
    CHECK(ina219_read_register(dev, REG_SHUNT_U, (uint16_t *)&raw));

    // Convert from raw value to voltage (10 µV per LSB)
    *voltage = raw * 0.00001;  // Convert to V

    return ESP_OK;
}

esp_err_t ina219_get_current(ina219_t *dev, float *current) {
    CHECK_ARG(dev && current);

    int16_t raw;
    CHECK(ina219_read_register(dev, REG_CURRENT, (uint16_t *)&raw));

    // Calculate current from raw value and LSB
    *current = raw * dev->i_lsb;

    return ESP_OK;
}

esp_err_t ina219_get_power(ina219_t *dev, float *power) {
    CHECK_ARG(dev && power);

    int16_t raw;
    CHECK(ina219_read_register(dev, REG_POWER, (uint16_t *)&raw));

    *power = raw * dev->p_lsb;

    return ESP_OK;
}

// INA219 Task
void ina219_task(void *pvParameters) {
    ina219_t *ina219 = (ina219_t *)pvParameters;

    if (ina219 == NULL || ina219->i2c_dev_handle == NULL) {
        ESP_LOGE(INA219_TAG, "INA219 initialization failed. Exiting task.");
        vTaskDelete(NULL);
    }

    // Configure the INA219 with appropriate settings
    esp_err_t err = ina219_configure(ina219, INA219_BUS_RANGE_32V, INA219_GAIN_1, INA219_RES_12BIT_128S, INA219_RES_12BIT_128S, INA219_MODE_CONT_SHUNT_BUS);
    if (err != ESP_OK) {
        ESP_LOGE(INA219_TAG, "Failed to configure INA219: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }

    // Calibrate the INA219 for expected current and shunt resistor value
    err = ina219_calibrate(ina219, 0.1, 1);  // Calibrated values: 0.1 ohm shunt resistor, 1.0A max current
    if (err != ESP_OK) {
        ESP_LOGE(INA219_TAG, "Failed to calibrate INA219: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }


    ESP_LOGI(INA219_TAG, "INA219 successfully configured and calibrated");

    // Wait for notification to start logging
    ESP_LOGI(INA219_TAG, "Waiting for start signal...");
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    float bus_voltage, shunt_voltage, current, power;

    while (1) {
        err = ina219_get_bus_voltage(ina219, &bus_voltage);
        if (err == ESP_OK) {
            // UPDATED: Changed to ESP_LOGD to reduce production noise
            ESP_LOGD(INA219_TAG, "Bus Voltage: %.2f V", bus_voltage);
        } else {
            ESP_LOGE(INA219_TAG, "Failed to read bus voltage: %s", esp_err_to_name(err));
        }

        err = ina219_get_shunt_voltage(ina219, &shunt_voltage);
        if (err == ESP_OK) {
            // UPDATED: Changed to ESP_LOGD to reduce production noise
            ESP_LOGD(INA219_TAG, "Shunt Voltage: %.2f mV", shunt_voltage * 1000);
        } else {
            ESP_LOGE(INA219_TAG, "Failed to read shunt voltage: %s", esp_err_to_name(err));
        }

        err = ina219_get_current(ina219, &current);
        if (err == ESP_OK) {
            // UPDATED: Changed to ESP_LOGD to reduce production noise
            ESP_LOGD(INA219_TAG, "Current: %.06f A", current);
        } else {
            ESP_LOGE(INA219_TAG, "Failed to read current: %s", esp_err_to_name(err));
        }

        err = ina219_get_power(ina219, &power);
        if (err == ESP_OK) {
            // UPDATED: Changed to ESP_LOGD to reduce production noise
            ESP_LOGD(INA219_TAG, "Power: %.06f W", power);
        } else {
            ESP_LOGE(INA219_TAG, "Failed to read power: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}