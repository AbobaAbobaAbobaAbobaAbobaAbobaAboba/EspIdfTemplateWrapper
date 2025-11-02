set(MY_ESP_COMPONENTS
    esp_http_server
    driver
    json
    esp_netif
    esp_event
    esp_wifi
    nvs_flash
    spiffs
    fatfs
    esp_driver_mcpwm
)

idf_component_register(SRCS "main.cpp"
 "wifi_wrapper/wifi_handler.cpp"
  "http_wrapper/http_server_wrapper.cpp"
   "spiffs_wrapper/spiffs_wrapper.cpp"
    "json_wrapper/json_wrapper.cpp"
    "gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.cpp"
    "websocket_handler/websocket_handler.cpp"
    "core/core.cpp"
    "gpio_wrappers/mcpwm_wrapper/mcpwm_wrapper.cpp"
     REQUIRES ${MY_ESP_COMPONENTS}
INCLUDE_DIRS ".")


target_compile_definitions(${COMPONENT_LIB} PUBLIC ENABLE_SAFENET)
target_compile_definitions(${COMPONENT_LIB} PUBLIC ENABLE_LOGS)
spiffs_create_partition_image(spiffs ../spiffs FLASH_IN_PROJECT)