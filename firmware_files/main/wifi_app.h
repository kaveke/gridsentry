// Header guard to prevent multiple inclusions
#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_wifi.h"
#include "esp_netif.h"

// Callback typedef
typedef void (*wifi_connected_event_callback_t)(void);

// WiFi application settings
// UPDATED: Using Kconfig values instead of hardcoded credentials
#define WIFI_AP_SSID			CONFIG_ESP_WIFI_SSID		// Access Point (AP) name
#define WIFI_AP_PASSWORD		CONFIG_ESP_WIFI_PASSWORD	// AP password
#define WIFI_AP_CHANNEL 		1				            // AP channel (frequency)
#define WIFI_AP_SSID_HIDDEN 	0				            // Set to 1 to hide the AP SSID
#define WIFI_AP_MAX_CONNECTIONS 5				            // Maximum number of clients
#define WIFI_AP_BEACON_INTERVAL 100				            // Beacon interval in milliseconds
#define WIFI_AP_IP 				"192.168.0.1"	            // Static IP address for the AP
#define WIFI_AP_GATEWAY 		"192.168.0.1" 	            // Gateway IP address for the AP
#define WIFI_AP_NETMASK			"255.255.255.0"             // Subnet mask for the AP
#define WIFI_AP_BANDWIDTH 		WIFI_BW_HT20	            // AP bandwidth: 20 MHz (minimal interference)
#define WIFI_STA_POWER_SAVE		WIFI_PS_NONE	            // Power-saving mode for station
#define MAX_SSID_LENGTH			32				            // Maximum SSID length
#define MAX_PASSWORD_LENGTH		64				            // Maximum password length
#define MAX_CONNECTION_RETRIES	5				            // Maximum retries for connection

// Network interface objects for station and AP modes
extern esp_netif_t *esp_netif_sta;
extern esp_netif_t *esp_netif_ap;

// Message IDs for the WiFi application's task queue
typedef enum wifi_app_message
{
    WIFI_APP_MSG_START_HTTP_SERVER = 0,          // Start HTTP server
    WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,    // Connection request from HTTP server
    WIFI_APP_MSG_STA_CONNECTED_GOT_IP,           // Station connected and obtained IP
    WIFI_APP_MSG_STA_DISCONNECTED,                 // Station disconnected 
    WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT,
    WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS
} wifi_app_message_e;

// Structure for task queue messages
typedef struct wifi_app_queue_message
{
    wifi_app_message_e msgID;                    // Message ID
} wifi_app_queue_message_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the wifi_app_message_e enum.
 * @return pdTRUE if an item was successfully sent to the queue, otherwise pdFALSE.
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

// Starts the WiFi RTOS task
void wifi_app_start(void);

/**
 * Gets the wifi configuration
 */
wifi_config_t* wifi_app_get_wifi_config(void);

/**
 * Sets the callback function
 */
void wifi_app_set_callback(wifi_connected_event_callback_t cb);

/**
 * Calls the callback function
 */
void wifi_app_call_callback(void);

#endif /* MAIN_WIFI_APP_H_ */