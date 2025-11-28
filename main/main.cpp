
#include "common/common.hpp"
#include "common/global.hpp"
#include "core/core.hpp"
#include "hardware/button_wrapper/button_wrapper.hpp"
#include "i2c_wrapper/i2c_wrapper.hpp"
#include "scheduler/scheduler.hpp"
#include "socket_wrappers/udp_wrapper/udp_wrapper.hpp"
#include <driver/gpio.h>
#include <iostream>
static void initBlink() {
    for (int i = 0; i < 10; ++i) {
        global::blinkLED(50);
    }
    for (int i = 0; i < 10; ++i) {
        global::blinkExternalLED(50);
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

void print_rssi() {
    wifi_ap_record_t ap_info;

    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        esp_log(kInfoLogConfig, "RSSI", "Current RSSI: %d dBm", ap_info.rssi);
    } else {
        esp_log(kErrorLogConfig, "RSSI", "Failed to get AP info");
    }
}

void true_main() {
#ifdef ENABLE_SAFENET
    try {
#endif

        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
        initializeSystem();
        global::initGlobalPins();

        verifySystem();

        initBlink();

        Core core;

        // Core core({"AbobaNet", "Aboba_425784_Net"});
        time_t now = time(nullptr);
        std::cout << "CORE START TIME: " << ctime(&now) << std::endl;
        UdpWrapper udp_wrapper;
        auto res = udp_wrapper.lock();
        std::string data = "Hello ABOBA !";
        ButtonWrapper wrapper(GPIO_NUM_0);

        skf_prext_GenericCommandMsg msg;
        msg.cmd_id = skf_printer_GenericCommandParachute;
        msg.parachute.activate = 1;

        wrapper.setShortPressCallback(
          []() { esp_log(kInfoLogConfig, "BUTTON", "Short press detected!"); });
        wrapper.setLongPressCallback([&]() {
            esp_log(kInfoLogConfig, "BUTTON", "Long press detected!");
            udp_wrapper.send("192.168.43.1", 13551, reinterpret_cast<char*>(&msg));
            // Scheduler::Scheduler::schedule(1'000'000, true, [&core]() {
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink1");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink2");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink3");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink4");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink5");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink6");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink7");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink8");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink9");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink10");
            //     global::blinkExternalLED(300);
            //     esp_log(kInfoLogConfig, "BUTTON", "Blink11");
            //     core.sleep(GPIO_NUM_0);
            // });
        });
        if (res != ESP_OK) {
            throw std::runtime_error("Failed to lock UDP wrapper");
        }

        while (true) {
            static int i = 0;
            ++i;
            vTaskDelay(pdMS_TO_TICKS(1500));
            // auto res =
            //   udp_wrapper.send("192.168.75.71", 12345, data + std::to_string(i));
            // if (res != ESP_OK) {
            //     esp_log(
            //       kErrorLogConfig,
            //       "UDP_TEST",
            //       "Failed to send UDP packet %d",
            //       res);
            // }
            // global::blinkLED(50);
            // global::blinkLED(50);
            // global::blinkLED(50);
            // global::blinkLED(50);
            // global::blinkLED(50);
            // global::blinkLED(50);
            // print_rssi();
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
