#ifndef WIFI_BASE_HPP
#define WIFI_BASE_HPP

#include "../common/common.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "wifi_group_wrapper.hpp"
#include <atomic>
#include <string>

namespace Wifi {
enum class Mode {
    STATION,
    AP
};

class Base {
  public:
    Base() {
#ifdef ENABLE_SAFENET
        if (instance_) {
            throw std::runtime_error("WiFi module already initialized");
        } else {
            instance_ = this;
        }
#endif
    }
    virtual ~Base() {
        if (instance_ == this) {
            deinit();
        }
    }
    inline static std::string getCurrentIp() {
        return ip_address;
    }
    virtual bool prepareWifi() = 0;
    virtual void init() = 0;
    inline static void deinit() {
        if (!instance_) {
            throw std::runtime_error(
              "WiFi module was attempted to be deinitialized without being "
              "initialized");
        }
        ip_address = "";
        instance_ = nullptr;
        auto res = esp_wifi_stop();

#ifdef ENABLE_SAFENET
        if (res != ESP_OK) {
            throw std::runtime_error("Failed to stop WiFi module");
        }
#endif
    }
    inline static bool isInit() {
        return instance_ != nullptr;
    }

    inline static void setTxPower(int8_t power) {
        if (power < 20 || power > 78) {
            esp_log(kErrorLogConfig, TAG, "TX power %d out of range (20-78)", power);
#ifdef ENABLE_SAFENET
            throw std::runtime_error(
              "TX power " + std::to_string(power) + " out of range (20-78)");
#endif
            return;
        }
        auto res = esp_wifi_set_max_tx_power(power);
        if (res != ESP_OK) {
            esp_log(
              kErrorLogConfig,
              TAG,
              "Failed to set TX power to %d: %s",
              power,
              esp_err_to_name(res));
#ifdef ENABLE_SAFENET
            throw std::runtime_error(
              "Failed to set TX power to " + std::to_string(power) + ": "
              + esp_err_to_name(res));
#endif
        }
    }

  protected:
    inline static std::string ip_address = "";
    inline static std::atomic<Wifi::Mode> mode;

  private:
    inline static Base* instance_ = nullptr;
    inline static constexpr TagType TAG = "WIFI_BASE";
};
};  // namespace Wifi
#endif