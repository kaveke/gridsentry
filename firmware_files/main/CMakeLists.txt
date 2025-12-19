idf_component_register(SRCS "aws_iot.c" "sntp_time_sync.c" "wifi_reset_button.c" "app_nvs.c" "ina219.c" "ina3221.c" "main.c" "rgb_led.c" "wifi_app.c" "http_server.c" "task_manager_i2c.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES "webpage/app.css" "webpage/app.js" "webpage/index.html" "webpage/favicon.ico" "webpage/jquery-3.3.1.min.js")
target_add_binary_data(${COMPONENT_TARGET} "certs/aws_root_ca_pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate_pem_crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private_pem_key" TEXT)