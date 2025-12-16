
#include "ble_wrapper/ble_wrapper.hpp"
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
        global::disableHold();
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

        wrapper.addHoldEvent(5'000'000, [&]() {
            esp_log(kInfoLogConfig, "BUTTON", "sleep press detected!");
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::blinkExternalLED(300);
            global::resetAndHoldPins();
            gpio_deep_sleep_hold_en();
            core.sleep(wrapper.getPin());
        });

        if (res != ESP_OK) {
            throw std::runtime_error("Failed to lock UDP wrapper");
        }

        BluetoothWrapper ble_wrapper;
        const ble_uuid128_t service_uuid = BLE_UUID128_INIT(
          0x2d,
          0x71,
          0xa2,
          0x59,
          0xb4,
          0x58,
          0xc8,
          0x12,
          0x99,
          0x99,
          0x43,
          0x95,
          0x00,
          0x00,
          0x37,
          0x13);
        const ble_uuid128_t char_uuid = BLE_UUID128_INIT(
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x00,
          0x37,
          0x13);
        ble_wrapper.setService(0, BLE_GATT_SVC_TYPE_PRIMARY, service_uuid);

        ble_wrapper.setCharacteristic(
          0,
          0,
          char_uuid,
          BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
          [](void* context) -> std::optional<std::vector<uint8_t>> {
              static int aboba = 0;
              ++aboba;
              global::blinkExternalLED(100);
              global::blinkExternalLED(100);
              global::blinkExternalLED(100);
              std::vector<uint8_t> res;
              res.resize(1);
              res[0] = aboba;
              return res;
          });
        ble_wrapper.start();

        wrapper.setShortPressCallback([&]() {
            esp_log(kInfoLogConfig, "BUTTON", "short press detected!");
            std::vector<uint8_t> res;
            res.resize(1);
            res[0] = 1;

            ble_wrapper.notifyAll(0, 0, res.data(), res.size());

            global::blinkExternalLED(75);
            global::blinkExternalLED(75);
            global::blinkExternalLED(75);
            global::blinkExternalLED(75);
        });

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
            global::blinkExternalLED(50);
            global::blinkExternalLED(50);
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
