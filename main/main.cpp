
#include "common/common.hpp"
#include "core/core.hpp"
#include <driver/gpio.h>
#include <iostream>
static void blinkLED(int interval = 100) {
    global::led_pin->write(1);
    vTaskDelay(pdMS_TO_TICKS(interval));
    global::led_pin->write(0);
    vTaskDelay(pdMS_TO_TICKS(interval));
}
static void initBlink() {
    for (int i = 0; i < 10; ++i) {
        blinkLED(50);
    }
}
static void verifySystem() {
    if (SystemStatus::getSystemStatus() != true) {
        esp_log(kErrorLogConfig, "FATAL", "System initialization failed!\n");
        while (true) {
            blinkLED(100);
        }
        return;
    }
}
void true_main() {
#ifdef ENABLE_SAFENET
    try {
#endif

        initializeSystem();
        global::initGlobalPins();

        verifySystem();

        initBlink();

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
extern "C" void app_main(void) {
    true_main();
}
