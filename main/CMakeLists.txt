set(MY_ESP_COMPONENTS
    esp_http_server
    driver
    cjson
    esp_netif
    esp_event
    esp_wifi
    nvs_flash
    fatfs
    esp_driver_mcpwm
    lwip
    littlefs
    esp_timer
    esp_driver_dac
)

idf_component_register(SRCS "main.cpp"
    "wifi_wrapper/station_wrapper.cpp"
    "wifi_wrapper/ap_wrapper.cpp"
    "http_wrapper/http_server_wrapper.cpp"
   "littlefs_wrapper/littlefs_wrapper.cpp"
    "json_wrapper/json_wrapper.cpp"
    "gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.cpp"
    "websocket_handler/websocket_handler.cpp"
    "core/core.cpp"
    "common/common.cpp"
    "sntp_wrapper/sntp_wrapper.cpp"
    "scheduler/scheduler.cpp"
    "i2c_wrapper/i2c_wrapper.cpp"
    "config_wrapper/config_wrapper.cpp"
    "socket_wrappers/udp_wrapper/udp_wrapper.cpp"
    "hardware/button_wrapper/button_wrapper.cpp"
     REQUIRES ${MY_ESP_COMPONENTS}
INCLUDE_DIRS ".")


target_compile_definitions(${COMPONENT_LIB} PUBLIC ENABLE_SAFENET)
target_compile_definitions(${COMPONENT_LIB} PUBLIC ENABLE_LOGS)
littlefs_create_partition_image(littlefs ../littlefs FLASH_IN_PROJECT)