/*
 * rgb_led.c
 *
 * Source file for RGB LED control using ESP32 LEDC peripheral.
 * Configures PWM channels and controls LED colors to indicate system states.
 *
 * Created on: Jan 1, 2025
 * Author: PC
 */

 #include <stdbool.h>
 #include <stdint.h>
 #include <stdio.h>
 #include "driver/ledc.h"
 #include "hal/ledc_types.h"
 #include "rgb_led.h"
 
 // Array to store LEDC channel configurations for Red, Green, and Blue
 ledc_info_t ledc_ch[RGB_LED_CHANNEL_NUM];
 
 // Flag to indicate whether PWM has been initialized
 bool g_pwm_init_handle = false;
 
 /*
  * Initialize the LEDC peripheral for controlling the RGB LED.
  * Configures each channel for PWM output and sets up the timer.
  */
 static void rgb_led_pwm_init(void) {
     // Initialize channel configurations
     ledc_ch[0] = (ledc_info_t){LEDC_CHANNEL_0, RGB_LED_RED_GPIO, LEDC_LOW_SPEED_MODE, LEDC_TIMER_0};  // Red
     ledc_ch[1] = (ledc_info_t){LEDC_CHANNEL_1, RGB_LED_GREEN_GPIO, LEDC_LOW_SPEED_MODE, LEDC_TIMER_0}; // Green
     ledc_ch[2] = (ledc_info_t){LEDC_CHANNEL_2, RGB_LED_BLUE_GPIO, LEDC_LOW_SPEED_MODE, LEDC_TIMER_0};  // Blue
 
     // Configure timer settings
     ledc_timer_config_t ledc_timer = {
         .duty_resolution = LEDC_TIMER_8_BIT,  // 8-bit resolution (0-255)
         .freq_hz = 100,                       // 100 Hz frequency
         .speed_mode = LEDC_LOW_SPEED_MODE,    // Low-speed mode
         .timer_num = LEDC_TIMER_0             // Use timer 0
     };
     if (ledc_timer_config(&ledc_timer) != ESP_OK) {
         printf("Failed to configure LEDC timer\n");
         return;
     }
 
     // Configure each channel
     for (int i = 0; i < RGB_LED_CHANNEL_NUM; i++) {
         ledc_channel_config_t ledc_channel = {
             .channel = ledc_ch[i].channel,
             .duty = 0,  // Initial duty cycle (LED off)
             .hpoint = 0,
             .gpio_num = ledc_ch[i].gpio,
             .intr_type = LEDC_INTR_DISABLE,
             .speed_mode = ledc_ch[i].mode,
             .timer_sel = ledc_ch[i].timer_index,
         };
         if (ledc_channel_config(&ledc_channel) != ESP_OK) {
             printf("Failed to configure LEDC channel %d\n", i);
             return;
         }
     }
     g_pwm_init_handle = true; // Mark initialization as complete
 }
 
 /*
  * Centralize initialization check to avoid redundant calls.
  */
 static void rgb_led_initialize(void) {
     if (!g_pwm_init_handle) {
         rgb_led_pwm_init();
     }
 }
 
 /*
  * Set the RGB LED color by adjusting the duty cycle of each channel.
  * Parameters: red, green, blue - Values (0-255) for each color channel.
  */
 static void rgb_led_set_colour(uint8_t red, uint8_t green, uint8_t blue) {
     esp_err_t err;
 
     err = ledc_set_duty(ledc_ch[0].mode, ledc_ch[0].channel, red);
     if (err != ESP_OK) printf("Failed to set red duty cycle: %s\n", esp_err_to_name(err));
     ledc_update_duty(ledc_ch[0].mode, ledc_ch[0].channel);
 
     err = ledc_set_duty(ledc_ch[1].mode, ledc_ch[1].channel, green);
     if (err != ESP_OK) printf("Failed to set green duty cycle: %s\n", esp_err_to_name(err));
     ledc_update_duty(ledc_ch[1].mode, ledc_ch[1].channel);
 
     err = ledc_set_duty(ledc_ch[2].mode, ledc_ch[2].channel, blue);
     if (err != ESP_OK) printf("Failed to set blue duty cycle: %s\n", esp_err_to_name(err));
     ledc_update_duty(ledc_ch[2].mode, ledc_ch[2].channel);
 }
 
 /* System State Indication Functions */
 void rgb_led_wifi_app_started(void) {
     rgb_led_initialize();
     rgb_led_set_colour(255, 0, 0); // Red
 }
 
 void rgb_led_http_server_started(void) {
     rgb_led_initialize();
     rgb_led_set_colour(0, 0, 255); // Blue
 }
 
 void rgb_led_wifi_connected(void) {
     rgb_led_initialize();
     rgb_led_set_colour(0, 255, 255); // Cyan
 }
 