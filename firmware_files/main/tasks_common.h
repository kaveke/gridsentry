// WiFi application task
#define WIFI_APP_TASK_STACK_SIZE        4096
#define WIFI_APP_TASK_PRIORITY          5
#define WIFI_APP_TASK_CORE_ID           0

// HTTP Server task
#define HTTP_SERVER_TASK_STACK_SIZE     8192
#define HTTP_SERVER_TASK_PRIORITY       4
#define HTTP_SERVER_TASK_CORE_ID        0

// HTTP Server Monitor task
#define HTTP_SERVER_MONITOR_STACK_SIZE  4096
#define HTTP_SERVER_MONITOR_PRIORITY    3
#define HTTP_SERVER_MONITOR_CORE_ID     0

// Wifi Reset Button Task
#define WIFI_RESET_BUTTON_STACK_SIZE    2048  
#define WIFI_RESET_BUTTON_TASK_PRIORITY 4
#define WIFI_RESET_BUTTON_TASK_CORE_ID  0

// INA219 Sensor Task
#define INA219_TASK_STACK_SIZE          4096  
#define INA219_TASK_PRIORITY            5
#define INA219_TASK_CORE_ID             1

// INA3221 Sensor Task
#define INA3221_TASK_STACK_SIZE         8192  
#define INA3221_TASK_PRIORITY           5
#define INA3221_TASK_CORE_ID            1

// SNTP Time Sync task
#define SNTP_TIME_SYNC_TASK_STACK_SIZE  8192  
#define SNTP_TIME_SYNC_TASK_PRIORITY    4
#define SNTP_TIME_SYNC_TASK_CORE_ID     1

// AWS IoT task
#define AWS_IOT_TASK_STACK_SIZE         12288
#define AWS_IOT_TASK_PRIORITY           4   
#define AWS_IOT_TASK_CORE_ID            1
