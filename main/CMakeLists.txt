idf_component_register(SRCS "main.cpp" "wifi_wrapper/wifi_handler.cpp" "http_wrapper/http_server_wrapper.cpp" "spiffs_wrapper/spiffs_wrapper.cpp"
                    INCLUDE_DIRS ".")


target_compile_definitions(${COMPONENT_LIB} PUBLIC ENABLE_SAFENET)
target_compile_definitions(${COMPONENT_LIB} PUBLIC ENABLE_LOGS)
spiffs_create_partition_image(spiffs ../spiffs FLASH_IN_PROJECT)