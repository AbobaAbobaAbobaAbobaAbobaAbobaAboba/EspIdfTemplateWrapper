
#include "common/common.hpp"
#include "gpio_wrapper/gpio_wrapper.hpp"
#include "http_wrapper/http_server_wrapper.hpp"
#include "spiffs_wrapper/spiffs_wrapper.hpp"
#include "wifi_wrapper/wifi_handler.hpp"
#include <driver/gpio.h>
#include <iostream>

static void blinkLED(int interval = 100) {
    led->write(1);
    vTaskDelay(pdMS_TO_TICKS(interval));
    led->write(0);
    vTaskDelay(pdMS_TO_TICKS(interval));
}
extern "C" void app_main(void) {
#ifdef ENABLE_SAFENET
    try {
#endif
        initializeSystem();

        if (SystemStatus::getSystemStatus() != true) {
            esp_log(kErrorLogConfig, "FATAL", "System initialization failed!\n");
            while (true) {
                blinkLED(100);
            }
            return;
        }

        WifiHandler wifi_handler;
        wifi_handler.connect("kgsn2g", "kbtaPR45ka$", 20);
        wifi_handler.waitForConnected();
        HttpServerWrapper http_server;
        http_server.startWebserver();
        while (true) {
            std::cout << "vremya tyanut rezinu..." << std::endl;
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
#ifdef ENABLE_SAFENET
    } catch (const std::exception& e) {
#endif

        while (true) {
            esp_log(
              kErrorLogConfig,
              "EXCEPTION",
              "%s"
              "\n",
              e.what());
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
#ifdef ENABLE_SAFENET
    }
#endif
}
