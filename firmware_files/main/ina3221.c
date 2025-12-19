/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Zaltora <https://github.com/Zaltora>
 * Copyright (c) 2019 Ruslan V. Uss <unclerus@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file ina3221.c
 *
 * ESP-IDF driver for Shunt and Bus Voltage Monitor INA3221
 *
 * Ported from esp-open-rtos
 *
 * Copyright (c) 2016 Zaltora <https://github.com/Zaltora>\n
 * Copyright (c) 2019 Ruslan V. Uss <unclerus@gmail.com>
 *
 * MIT Licensed as described in the file LICENSE
 */
#include <esp_log.h>
#include <esp_idf_lib_helpers.h>
#include <string.h>

#include "ina3221.h"
#include "task_manager_i2c.h"

#define SENSOR_INA3221
#include "config.h"

static const char *TAG = "ina3221";

#define I2C_TIMEOUT_MS 1000

#define INA3221_REG_CONFIG                      (0x00)
#define INA3221_REG_SHUNTVOLTAGE_1              (0x01)
#define INA3221_REG_BUSVOLTAGE_1                (0x02)
#define INA3221_REG_CRITICAL_ALERT_1            (0x07)
#define INA3221_REG_WARNING_ALERT_1             (0x08)
#define INA3221_REG_SHUNT_VOLTAGE_SUM           (0x0D)
#define INA3221_REG_SHUNT_VOLTAGE_SUM_LIMIT     (0x0E)
#define INA3221_REG_MASK                        (0x0F)
#define INA3221_REG_VALID_POWER_UPPER_LIMIT     (0x10)
#define INA3221_REG_VALID_POWER_LOWER_LIMIT     (0x11)

#define INA3221_SHUNT_RESISTOR_CH1 0.1f
#define INA3221_SHUNT_RESISTOR_CH2 0.1f
#define INA3221_SHUNT_RESISTOR_CH3 0.1f

#define WARNING_CHANNEL 1
#define WARNING_CURRENT (40.0)
#define INA3221_TAG "INA3221"

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

static esp_err_t read_reg_16(ina3221_t *dev, uint8_t reg, uint16_t *val)
{
    CHECK_ARG(val);

    uint8_t buf[2];
    CHECK(i2c_master_transmit_receive(dev->i2c_dev, &reg, 1, buf, 2, -1));
    *val = (buf[0] << 8) | buf[1];

    return ESP_OK;
}

static esp_err_t write_reg_16(ina3221_t *dev, uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = { reg, (uint8_t)(val >> 8), (uint8_t)(val & 0xFF) };
    CHECK(i2c_master_transmit(dev->i2c_dev, buf, 3, -1));

    return ESP_OK;
}

static esp_err_t write_config(ina3221_t *dev)
{
    return write_reg_16(dev, INA3221_REG_CONFIG, dev->config.config_register);
}

static esp_err_t write_mask(ina3221_t *dev)
{
    return write_reg_16(dev, INA3221_REG_MASK, dev->mask.mask_register & INA3221_MASK_CONFIG);
}

///////////////////////////////////////////////////////////////////////////////////

esp_err_t ina3221_free_desc(ina3221_t *dev) {
    CHECK_ARG(dev);

    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev->i2c_dev));
    ESP_ERROR_CHECK(i2c_del_master_bus(dev->i2c_bus_handle));

    return ESP_OK;
}

esp_err_t ina3221_init(i2c_master_bus_handle_t i2c_bus, ina3221_t *ina3221, uint8_t i2c_address) {
    if (!ina3221) {
        ESP_LOGE(TAG, "INA3221 handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (i2c_bus == NULL) {
        ESP_LOGE(TAG, "I2C bus handle is NULL. Did you call task_manager_i2c_init_desc()?");
        return ESP_ERR_INVALID_STATE;
    }

    memset(ina3221, 0, sizeof(ina3221_t));
    ina3221->i2c_addr = i2c_address;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_address,
        .scl_speed_hz = 100000,
        .scl_wait_us = I2C_TIMEOUT_MS
    };

    esp_err_t ret = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &ina3221->i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add INA3221 device to I2C bus, error: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "INA3221 initialized at address 0x%02X", i2c_address);
    return ESP_OK;
}


esp_err_t ina3221_sync(ina3221_t *dev)
{
    CHECK_ARG(dev);

    uint16_t data;

    // Sync config register
    CHECK(read_reg_16(dev, INA3221_REG_CONFIG, &data));
    if (data != dev->config.config_register)
        CHECK(write_config(dev));

    // Sync mask register
    CHECK(read_reg_16(dev, INA3221_REG_MASK, &data));
    if ((data & INA3221_MASK_CONFIG) != (dev->mask.mask_register & INA3221_MASK_CONFIG))
        CHECK(write_mask(dev));

    return ESP_OK;
}

esp_err_t ina3221_trigger(ina3221_t *dev)
{
    return write_config(dev);
}

esp_err_t ina3221_get_status(ina3221_t *dev)
{
    return read_reg_16(dev, INA3221_REG_MASK, &dev->mask.mask_register);
}

esp_err_t ina3221_set_options(ina3221_t *dev, bool mode, bool bus, bool shunt)
{
    CHECK_ARG(dev);

    dev->config.mode = mode;
    dev->config.ebus = bus;
    dev->config.esht = shunt;
    return write_config(dev);
}

esp_err_t ina3221_enable_channel(ina3221_t *dev, bool ch1, bool ch2, bool ch3)
{
    CHECK_ARG(dev);

    dev->config.ch1 = ch1;
    dev->config.ch2 = ch2;
    dev->config.ch3 = ch3;
    return write_config(dev);
}

esp_err_t ina3221_enable_channel_sum(ina3221_t *dev, bool ch1, bool ch2, bool ch3)
{
    CHECK_ARG(dev);

    dev->mask.scc1 = ch1;
    dev->mask.scc2 = ch2;
    dev->mask.scc3 = ch3;
    return write_mask(dev);
}

esp_err_t ina3221_enable_latch_pin(ina3221_t *dev, bool warning, bool critical)
{
    CHECK_ARG(dev);

    dev->mask.wen = warning;
    dev->mask.cen = critical;
    return write_mask(dev);
}

esp_err_t ina3221_set_average(ina3221_t *dev, ina3221_avg_t avg)
{
    CHECK_ARG(dev);

    dev->config.avg = avg;
    return write_config(dev);
}

esp_err_t ina3221_set_bus_conversion_time(ina3221_t *dev, ina3221_ct_t ct)
{
    CHECK_ARG(dev);

    dev->config.vbus = ct;
    return write_config(dev);
}

esp_err_t ina3221_set_shunt_conversion_time(ina3221_t *dev, ina3221_ct_t ct)
{
    CHECK_ARG(dev);

    dev->config.vsht = ct;
    return write_config(dev);
}

esp_err_t ina3221_reset(ina3221_t *dev)
{
    CHECK_ARG(dev);

    dev->config.config_register = INA3221_DEFAULT_CONFIG;
    dev->mask.mask_register = INA3221_DEFAULT_CONFIG;
    dev->config.rst = 1;
    return write_config(dev);
}

esp_err_t ina3221_get_bus_voltage(ina3221_t *dev, ina3221_channel_t channel, float *voltage)
{
    CHECK_ARG(dev && voltage);

    int16_t raw;

    CHECK(read_reg_16(dev, INA3221_REG_BUSVOLTAGE_1 + channel * 2, (uint16_t *)&raw));
    *voltage = raw * 0.001;

    return ESP_OK;
}

esp_err_t ina3221_get_shunt_value(ina3221_t *dev, ina3221_channel_t channel, float *voltage, float *current)
{
    CHECK_ARG(dev);
    CHECK_ARG(voltage || current);
    
    if (current && !dev->shunt[channel])
    {
        ESP_LOGE(TAG, "No shunt configured for channel %u in device [0x%02X on bus %p]", 
                 channel, dev->i2c_addr, (void *)dev->i2c_bus_handle);
        return ESP_ERR_INVALID_ARG;
    }

    int16_t raw;
    CHECK(read_reg_16(dev, INA3221_REG_SHUNTVOLTAGE_1 + channel * 2, (uint16_t *)&raw));
    float mvolts = raw * 0.005; // mV, 40uV step

    if (voltage)
        *voltage = mvolts;

    if (current)
        *current = mvolts * 1000.0 / dev->shunt[channel];  // mA

    //ESP_LOGI(TAG, "Channel %u: Voltage = %.3f mV, Current = %.3f mA", channel, mvolts, current ? *current : 0.0);

    return ESP_OK;
}

esp_err_t ina3221_get_sum_shunt_value(ina3221_t *dev, float *voltage)
{
    CHECK_ARG(dev && voltage);

    int16_t raw;

    CHECK(read_reg_16(dev, INA3221_REG_SHUNT_VOLTAGE_SUM, (uint16_t *)&raw));
    *voltage = raw * 0.02; // mV

    return ESP_OK;
}

esp_err_t ina3221_set_critical_alert(ina3221_t *dev, ina3221_channel_t channel, float current)
{
    CHECK_ARG(dev);

    int16_t raw = current * dev->shunt[channel] * 0.2;
    return write_reg_16(dev, INA3221_REG_CRITICAL_ALERT_1 + channel * 2, *(uint16_t *)&raw);
}

esp_err_t ina3221_set_warning_alert(ina3221_t *dev, ina3221_channel_t channel, float current)
{
    CHECK_ARG(dev);

    int16_t raw = current * dev->shunt[channel] * 0.2;
    return write_reg_16(dev, INA3221_REG_WARNING_ALERT_1 + channel * 2, *(uint16_t *)&raw);
}

esp_err_t ina3221_set_sum_warning_alert(ina3221_t *dev, float voltage)
{
    CHECK_ARG(dev);

    int16_t raw = voltage * 50.0;
    return write_reg_16(dev, INA3221_REG_SHUNT_VOLTAGE_SUM_LIMIT, *(uint16_t *)&raw);
}

esp_err_t ina3221_set_power_valid_upper_limit(ina3221_t *dev, float voltage)
{
    CHECK_ARG(dev);

    if (!dev->config.ebus)
    {
        ESP_LOGE(TAG, "Bus is not enabled in device [0x%02x on bus %p]", dev->i2c_addr, (void *)dev->i2c_bus_handle);
        return ESP_ERR_NOT_SUPPORTED;
    }

    int16_t raw = (int16_t)(voltage * 1000.0f);

    return write_reg_16(dev, INA3221_REG_VALID_POWER_UPPER_LIMIT, (uint16_t)raw);
}

esp_err_t ina3221_set_power_valid_lower_limit(ina3221_t *dev, float voltage)
{
    CHECK_ARG(dev);

    if (!dev->config.ebus)
    {
        ESP_LOGE(TAG, "Bus is not enabled in device [0x%02x on bus %p]", dev->i2c_addr, (void *)dev->i2c_bus_handle);
        return ESP_ERR_NOT_SUPPORTED;
    }

    int16_t raw = (int16_t)(voltage * 1000.0f);

    return write_reg_16(dev, INA3221_REG_VALID_POWER_LOWER_LIMIT, (uint16_t)raw);
}

void ina3221_task(void *pvParameters) {
    ina3221_t *dev = (ina3221_t *)pvParameters;

    if (dev == NULL || dev->i2c_dev == NULL) {
        ESP_LOGE(INA3221_TAG, "INA3221 initialization failed. Exiting task.");
        vTaskDelete(NULL);
    }

    // Configure INA3221 settings
    esp_err_t err = ina3221_set_options(dev, true, true, true); // Mode selection, bus and shunt activated
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set INA3221 options: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }

    err = ina3221_enable_channel(dev, true, true, true); // Enable all channels
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to enable INA3221 channels: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }

    float shunt_resistors[INA3221_BUS_NUMBER] = {100, 100, 100};

    // Assign shunt resistor values
    for (int i = 0; i < INA3221_BUS_NUMBER; i++) {
        dev->shunt[i] = shunt_resistors[i];
        ESP_LOGI(INA3221_TAG, "Shunt resistor for channel %d set to %.2f Ω", i + 1, (double)dev->shunt[i]);
    }

    err = ina3221_set_average(dev, INA3221_AVG_64); // 64 samples average
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set INA3221 averaging: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }

    err = ina3221_set_bus_conversion_time(dev, INA3221_CT_2116); // 2ms by channel (bus)
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set INA3221 bus conversion time: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }

    err = ina3221_set_shunt_conversion_time(dev, INA3221_CT_2116); // 2ms by channel (shunt)
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set INA3221 shunt conversion time: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }

    err = ina3221_set_warning_alert(dev, WARNING_CHANNEL - 1, WARNING_CURRENT);
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set warning alert: %s", esp_err_to_name(err));
        vTaskDelete(NULL);
    }

    ESP_LOGI(INA3221_TAG, "INA3221 successfully configured");

    // Wait for notification to start logging
    ESP_LOGI(INA3221_TAG, "Waiting for start signal...");
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    float bus_voltage, shunt_voltage, shunt_current;

    while (1) {
        for (uint8_t i = 0; i < INA3221_BUS_NUMBER; i++) {
            err = ina3221_get_bus_voltage(dev, i, &bus_voltage);
            err |= ina3221_get_shunt_value(dev, i, &shunt_voltage, &shunt_current);
    
            if (err == ESP_OK && (bus_voltage > 0.0 || shunt_current > 0.0)) {
                // UPDATED: Changed to ESP_LOGD to reduce production noise
                ESP_LOGD(INA3221_TAG, "C%u: Bus Voltage: %.2f V", i + 1, bus_voltage);
                ESP_LOGD(INA3221_TAG, "C%u: Shunt Voltage: %.2f mV", i + 1, shunt_voltage);
                ESP_LOGD(INA3221_TAG, "C%u: Shunt Current: %.2f mA\n", i + 1, shunt_current);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}