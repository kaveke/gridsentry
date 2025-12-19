#pragma once

/********************************************************
 *                      COMMON PARAMETERS               *
 ********************************************************/
// I2C Bus Configuration
#define I2C_PORT I2C_NUM_0  // Can be changed if using another I2C port
#define I2C_MASTER_SDA 21
#define I2C_MASTER_SCL 22
#define I2C_MASTER_FREQ_HZ 100000  // 400kHz

// Shunt Resistor Value (only relevant for INA219 and INA3221)
#define SHUNT_RESISTOR_MILLI_OHM 100

// Warning Thresholds (common across sensors)
#define WARNING_CHANNEL 1

/********************************************************
 *                      INA3221 PARAMETERS              *
 ********************************************************/
#ifdef SENSOR_INA3221
    #define I2C_ADDR_3221 0x40  // INA3221 Default I2C Address
    #define INA3221_CHANNEL_COUNT 3  // INA3221 has 3 channels
    #define INA3221_WARNING_VOLTAGE 12.0f  // Warning voltage threshold

    // INA3221 Specific Channel Enable Options
    #define INA3221_ENABLE_CHANNEL_1 1
    #define INA3221_ENABLE_CHANNEL_2 1
    #define INA3221_ENABLE_CHANNEL_3 1
#endif

/********************************************************
 *                      INA219 PARAMETERS               *
 ********************************************************/
#ifdef SENSOR_INA219
    #define I2C_ADDR_219 0x41  // INA219 Default I2C Address
    #define INA219_MAX_CURRENT 3200  // Max measurable current in mA
    #define INA219_CALIBRATION_VALUE 4096  // Calibration value for INA219
#endif

/********************************************************
 *                      INA260 PARAMETERS               *
 ********************************************************/
#ifdef SENSOR_INA260
    #define I2C_ADDR_260 0x42  // INA260 Default I2C Address
    #define INA260_MAX_VOLTAGE 36.0f  // Max voltage the sensor can handle
    #define INA260_ALERT_THRESHOLD 1.5f  // Alert threshold in Amps
#endif

/********************************************************
 *                      FUNCTIONAL PARAMETERS           *
 ********************************************************/
// Polling intervals for sensor readings
#define POLLING_INTERVAL_MS 1000

// Debugging option
#define ENABLE_DEBUG 1  // Set to 0 to disable debug output

/********************************************************
 *                      ERROR HANDLING                  *
 ********************************************************/
#if !defined(SENSOR_INA3221) && !defined(SENSOR_INA219) && !defined(SENSOR_INA260)
    #error "No sensor defined! Define SENSOR_INA3221, SENSOR_INA219, or SENSOR_INA260 before including this config."
#endif

/********************************************************
 *                      LED CONFIGURATION               *
 ********************************************************/
#define LED_GPIO GPIO_NUM_2  // Adjust to your actual pin
