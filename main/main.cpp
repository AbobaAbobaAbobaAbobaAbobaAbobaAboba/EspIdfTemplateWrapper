
#include "common/common.hpp"
#include "common/global.hpp"
#include "core/core.hpp"
#include "i2c_wrapper/i2c_wrapper.hpp"
#include "scheduler/scheduler.hpp"
#include <driver/gpio.h>
#include <iostream>
static void initBlink() {
    for (int i = 0; i < 10; ++i) {
        global::blinkLED(50);
    }
}
static void verifySystem() {
    if (SystemStatus::getSystemStatus() != true) {
        esp_log(kErrorLogConfig, "FATAL", "System initialization failed!\n");
        while (true) {
            global::blinkLED(100);
            LittleFS::LittleFsWrapper::printFilesystemInfo();
            LittleFS::LittleFsWrapper::printFiles();
        }
        return;
    }
}

// Example usage (in your main.cpp or elsewhere):
/*
I2CWrapper i2c(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22, 100000);
if (i2c.init() == ESP_OK) {
    uint8_t data_to_send[] = {0x01, 0x02};
    i2c.write(0x3C, data_to_send, sizeof(data_to_send));  // Send to slave 0x3C

    uint8_t data_read[2];
    i2c.read(0x3C, data_read, sizeof(data_read));  // Read from slave 0x3C

    // For register read: write reg addr 0x00 then read 1 byte
    uint8_t reg = 0x00;
    uint8_t value;
    i2c.writeThenRead(0x3C, &reg, 1, &value, 1);
}
*/

void true_main() {
#ifdef ENABLE_SAFENET
    try {
#endif

        initializeSystem();
        global::initGlobalPins();

        verifySystem();

        initBlink();

        Core core({"kgsn2g", "kbtaPR45ka$"});
        // Core core({"AbobaNet", "Aboba_425784_Net"});
        time_t now = time(nullptr);
        std::cout << "CORE START TIME: " << ctime(&now) << std::endl;

        I2CWrapper i2c(I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22, 100000);
        uint16_t state = 0b0000'0000'0000'0001;
        i2c.write(0x20, reinterpret_cast<uint8_t*>(&state), sizeof(state));
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1500));
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
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
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
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
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
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    } catch (...) {
        while (true) {
            esp_log(kErrorLogConfig, "EXCEPTION", "Unknown exception occurred\n");
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            global::blinkLED(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
        };
    }
#endif
}
extern "C" void app_main(void) {
    true_main();
}
