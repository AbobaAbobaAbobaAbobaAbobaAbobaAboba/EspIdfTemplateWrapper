#ifndef WIFI_GROUP_WRAPPER_HPP
#define WIFI_GROUP_WRAPPER_HPP
#include "../common/common.hpp"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
class WifiGroupWrapper {
  public:
    WifiGroupWrapper() {
        wifi_event_group_ = xEventGroupCreate();
        if (wifi_event_group_ == nullptr) {
            esp_log(
              kInfoLogConfig,
              "WIFI_GROUP",
              "Failed to create Wifi event group!\n");
        }
    }
    ~WifiGroupWrapper() {
        if (wifi_event_group_ != nullptr) {
            vEventGroupDelete(wifi_event_group_);
        }
    }
    // Delete copy and move operations
    WifiGroupWrapper(const WifiGroupWrapper& other) = delete;
    WifiGroupWrapper& operator=(const WifiGroupWrapper& other) = delete;
    WifiGroupWrapper(WifiGroupWrapper&& other) = delete;
    WifiGroupWrapper& operator=(WifiGroupWrapper&& other) = delete;

    void setBits(int bits) {
        // if (event_group_ != nullptr) {
        //     xEventGroupSetBits(event_group_, bits);
        // }
    }

    [[nodiscard]] EventBits_t waitConnectedBits() {
        return xEventGroupWaitBits(
          wifi_event_group_,
          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
          pdFALSE,
          pdFALSE,
          portMAX_DELAY);
    }

  private:
    friend class WifiHandler;
    EventGroupHandle_t& get() { return wifi_event_group_; }
    EventGroupHandle_t wifi_event_group_;
};
#endif  // WIFI_GROUP_WRAPPER_HPP