
#include "common/common.hpp"
#include "core/core.hpp"
#include <driver/gpio.h>
#include <iostream>

static void blinkLED(int interval = 100) {
    // led->write(1);
    // vTaskDelay(pdMS_TO_TICKS(interval));
    // led->write(0);
    // vTaskDelay(pdMS_TO_TICKS(interval));
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

        Core core;
        while (true) {
            std::cout << "vremya tyanut rezinu..." << std::endl;
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
#ifdef ENABLE_SAFENET
    } catch (const std::exception& e) {
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
    } catch (const char* msg) {
        while (true) {
            esp_log(
              kErrorLogConfig,
              "EXCEPTION",
              "%s"
              "\n",
              msg);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    } catch (int code) {
        while (true) {
            esp_log(
              kErrorLogConfig,
              "EXCEPTION",
              "%d"
              "\n",
              code);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    } catch (...) {
        while (true) {
            esp_log(kErrorLogConfig, "EXCEPTION", "Unknown exception occurred\n");
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    }
#endif
}
