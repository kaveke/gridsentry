#include "task_manager_i2c.h"
#include "driver/gpio.h"
#include "freertos/task.h"

#define WARNING_CURRENT (40.0)

static const char *TAG = "task_manager_i2c";
static const char *INA219_TAG = "AWS_INA219";
static const char *INA3221_TAG = "AWS_INA3221";

static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static bool i2c_initialized = false;

static TaskHandle_t ina219_handle = NULL;
static TaskHandle_t ina3221_handle = NULL;

esp_err_t task_manager_i2c_init(void) {
    if (i2c_initialized) {
        ESP_LOGW(TAG, "I2C already initialized");
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_io_num = I2C_MASTER_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    esp_err_t err = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C master bus: %s", esp_err_to_name(err));
        return err;
    }

    i2c_initialized = true;
    ESP_LOGI(TAG, "I2C master bus initialized");
    return ESP_OK;
}

void i2c_scan() {
    // UPDATED: Replaced printf with ESP_LOGI for better logging consistency
    ESP_LOGI(TAG, "I2C scanning...");

    esp_log_level_set("i2c.master", ESP_LOG_NONE);

    for (uint8_t addr = 0x40; addr < 0x4F; addr++) {
        esp_err_t ret = i2c_master_probe(I2C_PORT, addr, 1000 / portTICK_PERIOD_MS);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "I2C device found at address 0x%02x", addr);
        }
    }

    esp_log_level_set("i2c.master", ESP_LOG_INFO);
    // UPDATED: Replaced printf with ESP_LOGI
    ESP_LOGI(TAG, "I2C scan complete");
}

void sensor_task_manager(void) {
    esp_err_t err = task_manager_i2c_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(err));
        return;
    }

    i2c_scan();

#ifdef SENSOR_INA3221
    ina3221_t *ina3221 = malloc(sizeof(ina3221_t));
    if (ina3221 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for INA3221");
        return;
    }

    esp_err_t ret = ina3221_init(i2c_bus_handle, ina3221, I2C_ADDR_3221);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize INA3221: %s", esp_err_to_name(ret));
        free(ina3221);
        return;
    }

    xTaskCreatePinnedToCore(ina3221_task, "INA3221_task", INA3221_TASK_STACK_SIZE, (void *)ina3221, INA3221_TASK_PRIORITY, &ina3221_handle, INA3221_TASK_CORE_ID);
#endif

#ifdef SENSOR_INA219
    ina219_t *ina219 = malloc(sizeof(ina219_t));
    if (ina219 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for INA219");
        return;
    }

    ret = ina219_init(i2c_bus_handle, ina219, I2C_ADDR_219);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize INA219: %s", esp_err_to_name(ret));
        free(ina219);
        return;
    }

    xTaskCreatePinnedToCore(ina219_task, "ina219_task", INA219_TASK_STACK_SIZE, (void *)ina219, INA219_TASK_PRIORITY, &ina219_handle, INA219_TASK_CORE_ID);
#endif

    ESP_LOGI(TAG, "All sensors initialized and configured. Starting data logging....");

    // Notify each sensor task to start logging
#ifdef SENSOR_INA219
    xTaskNotifyGive(ina219_handle);
#endif
#ifdef SENSOR_INA3221
    xTaskNotifyGive(ina3221_handle);
#endif
}

// Function to initialize and configure both INA219 and INA3221 sensors
esp_err_t initialize_sensors(ina219_t *ina219, ina3221_t *ina3221) {
    esp_err_t err = task_manager_i2c_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(err));
    }

    i2c_scan();

    esp_err_t ret = ina3221_init(i2c_bus_handle, ina3221, I2C_ADDR_3221);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize INA3221: %s", esp_err_to_name(ret));
        free(ina3221);
    }

    ret = ina219_init(i2c_bus_handle, ina219, I2C_ADDR_219);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize INA219: %s", esp_err_to_name(ret));
        free(ina219);
    }

    // Configure INA219
    err = ina219_configure(ina219, INA219_BUS_RANGE_32V, INA219_GAIN_1, INA219_RES_12BIT_128S, INA219_RES_12BIT_128S, INA219_MODE_CONT_SHUNT_BUS);
    if (err != ESP_OK) {
        ESP_LOGE(INA219_TAG, "Failed to configure INA219: %s", esp_err_to_name(err));
        return err;
    }

    // Calibrate INA219
    err = ina219_calibrate(ina219, 0.1, 3.2);  // Calibrated values: 0.1 ohm shunt resistor, 1.0A max current
    if (err != ESP_OK) {
        ESP_LOGE(INA219_TAG, "Failed to calibrate INA219: %s", esp_err_to_name(err));
        return err;
    }

    // Initialize INA3221
    err = ina3221_init(i2c_bus_handle, ina3221, I2C_ADDR_3221);
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to initialize INA3221: %s", esp_err_to_name(err));
        return err;
    }

    // Set INA3221 options
    err = ina3221_set_options(ina3221, true, true, true); // Mode selection, bus and shunt activated
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set INA3221 options: %s", esp_err_to_name(err));
        return err;
    }

    // Enable INA3221 channels
    err = ina3221_enable_channel(ina3221, true, true, true); // Enable all channels
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to enable INA3221 channels: %s", esp_err_to_name(err));
        return err;
    }

    // Assign shunt resistor values to INA3221
    float shunt_resistors[INA3221_BUS_NUMBER] = {100, 100, 100};
    for (int i = 0; i < INA3221_BUS_NUMBER; i++) {
        ina3221->shunt[i] = shunt_resistors[i];
        ESP_LOGI(INA3221_TAG, "Shunt resistor for channel %d set to %.2f Ω", i + 1, (double)ina3221->shunt[i]);
    }

    // Set INA3221 averaging
    err = ina3221_set_average(ina3221, INA3221_AVG_64); // 64 samples average
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set INA3221 averaging: %s", esp_err_to_name(err));
        return err;
    }

    // Set INA3221 bus and shunt conversion times
    err = ina3221_set_bus_conversion_time(ina3221, INA3221_CT_2116); // 2ms by channel (bus)
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set INA3221 bus conversion time: %s", esp_err_to_name(err));
        return err;
    }

    err = ina3221_set_shunt_conversion_time(ina3221, INA3221_CT_2116); // 2ms by channel (shunt)
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set INA3221 shunt conversion time: %s", esp_err_to_name(err));
        return err;
    }

    // Set warning alert for INA3221
    err = ina3221_set_warning_alert(ina3221, WARNING_CHANNEL - 1, WARNING_CURRENT);
    if (err != ESP_OK) {
        ESP_LOGE(INA3221_TAG, "Failed to set warning alert: %s", esp_err_to_name(err));
        return err;
    }

    // Initialise theft LED
    led_gpio_init();

    return ESP_OK;
}

void led_gpio_init(void) {
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0); // LED off initially
}