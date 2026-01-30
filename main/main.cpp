
#include "common/common.hpp"
#include "common/global.hpp"
#include "core/core.hpp"
#include "driver/mcpwm_types.h"
#include "gpio_wrappers/mcpwm_wrapper/mcpwm_wrapper.hpp"
#include "hardware/button_wrapper/button_wrapper.hpp"
#include "i2c_wrapper/i2c_wrapper.hpp"
#include "scheduler/scheduler.hpp"
#include "socket_wrappers/udp_wrapper/udp_wrapper.hpp"
#include "uart_wrapper/uart_wrapper.hpp"
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
void printHeapStats(void) {
    size_t free_internal =
      heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    size_t largest_internal =
      heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

    // If you have PSRAM enabled:
    // size_t free_psram    = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    // size_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

    ESP_LOGI(
      "ABOBA",
      "Internal heap → free: %u B   largest block: %u B",
      free_internal,
      largest_internal);

    // Bonus: minimum ever free (needs CONFIG_ESP_HEAP_TRACING_STANDALONE or similar in
    // menuconfig) But usually people just track free + largest
}
void true_main() {
#ifdef ENABLE_SAFENET
    try {
#endif
        initializeSystem();
        global::initGlobalPins();
        verifySystem();

        initBlink();

        uart_config_t uart_config = {
          .baud_rate = 115200,                    // Common baud rate
          .data_bits = UART_DATA_8_BITS,          // 8 data bits
          .parity = UART_PARITY_DISABLE,          // No parity
          .stop_bits = UART_STOP_BITS_1,          // 1 stop bit
          .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // No hardware flow control
          .rx_flow_ctrl_thresh = 0,               // Not used with flow control
          .source_clk = UART_SCLK_DEFAULT,        // Default clock source
          .flags = {0, 0}                         // No signal inversions or other
        };
        // UartWrapper uart_wrapper(UART_NUM_1, 17, 16, uart_config);

        Core core;

        // Core core({"AbobaNet", "Aboba_425784_Net"});
        time_t now = time(nullptr);
        std::cout << "CORE START TIME: " << ctime(&now) << std::endl;
        UdpWrapper udp_wrapper;
        auto res = udp_wrapper.lock();
        std::string data = "Hello ABOBA !";
        ButtonWrapper wrapper(GPIO_NUM_0);

        wrapper.addHoldEvent(5'000'000, [&]() {
            esp_log(kInfoLogConfig, "BUTTON", "sleep press detected!");
            global::blinkLED(300);
            global::blinkLED(300);
            global::blinkLED(300);
            global::blinkLED(300);
            global::blinkLED(300);
        });

        if (res != ESP_OK) {
            throw std::runtime_error("Failed to lock UDP wrapper");
        }

        std::shared_ptr<GPIO::GpioBasicWrapper> enable_pin_ =
          GPIO::GpioBasicWrapper::createPin(
            GPIO_NUM_18,
            GPIO_MODE_OUTPUT,
            GPIO::PullMode::PULL_UP);

        std::shared_ptr<GPIO::GpioBasicWrapper> dir_pin_ =
          GPIO::GpioBasicWrapper::createPin(
            GPIO_NUM_5,
            GPIO_MODE_OUTPUT,
            GPIO::PullMode::PULL_UP);

        std::shared_ptr<GPIO::McpwmTimer> timer_ =
          GPIO::McpwmTimer::createTimer(0, 100);

        std::shared_ptr<GPIO::McpwmWrapper> motor_pin =
          GPIO::McpwmWrapper::createPin(GPIO_NUM_4, timer_);

        while (true) {
            global::blinkLED(50);
            global::blinkLED(50);
            static bool aboba_bool = false;
            aboba_bool = !aboba_bool;
            if (aboba_bool) {
                static bool dir = true;
                dir = !dir;
                vTaskDelay(pdMS_TO_TICKS(10));
                enable_pin_->write(0);
                vTaskDelay(pdMS_TO_TICKS(10));
                dir_pin_->write(dir ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(10));
                motor_pin->write(50);
                esp_log(kErrorLogConfig, "ABOBA", "Rotating! %d", dir);
            }
            vTaskDelay(pdMS_TO_TICKS(750));
            if (aboba_bool) {
                enable_pin_->write(1);
                motor_pin->stop(MCPWM_TIMER_START_STOP_EMPTY);
            }

            printHeapStats();
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
