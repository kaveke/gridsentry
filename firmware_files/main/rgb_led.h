/*
 * rgb_led.h
 *
 * Header file for controlling an RGB LED using ESP32 LEDC peripheral.
 * Provides GPIO configuration and declarations for state-specific RGB indications.
 *
 * Created on: Jan 1, 2025
 * Author: PC
 */

#ifndef MAIN_RGB_LED_H_
#define MAIN_RGB_LED_H_

// Define GPIO pins for the RGB LED
#define RGB_LED_RED_GPIO       13  // GPIO pin for Red channel
#define RGB_LED_GREEN_GPIO     12  // GPIO pin for Green channel
#define RGB_LED_BLUE_GPIO      14  // GPIO pin for Blue channel

// Number of RGB LED channels
#define RGB_LED_CHANNEL_NUM    3

// Struct for RGB LED channel configuration
typedef struct {
    int channel;       // LEDC channel number
    int gpio;          // GPIO pin number
    int mode;          // Speed mode (high/low speed)
    int timer_index;   // Timer index used for the channel
} ledc_info_t;

// Function prototypes for indicating system states using RGB LED
void rgb_led_wifi_app_started(void);    // Indicate WiFi app started (Red)
void rgb_led_http_server_started(void); // Indicate HTTP server started (Blue)
void rgb_led_wifi_connected(void);      // Indicate WiFi connected (Cyan)

#endif /* MAIN_RGB_LED_H_ */
