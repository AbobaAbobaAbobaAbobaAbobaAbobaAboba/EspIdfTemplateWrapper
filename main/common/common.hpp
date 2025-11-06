#ifndef COMMON_HPP
#define COMMON_HPP
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <iostream>

typedef const char* TagType;
static constexpr int WIFI_CONNECTED_BIT = BIT0;
static constexpr int WIFI_FAIL_BIT = BIT1;

struct Credentials {
    std::string ssid;
    std::string password;
};
inline void initializeWifiStack();
struct SystemStatus {
  public:
    inline static bool getSystemStatus() { return system_status_; }
    inline static bool getWifiStatus() { return wifi_status_; }

  private:
    friend void initializeSystem();
    friend void initializeWifiStack();
    inline static bool system_status_ = false;
    inline static bool wifi_status_ = false;
};

struct AssignedIp {
    inline static std::string getCurrentIp() { return ip_address; }

  private:
    friend class WifiHandler;
    inline static std::string ip_address;
};
constexpr esp_log_config_t kInfoLogConfig = {
  .opts = {
    .log_level = ESP_LOG_INFO,
    .constrained_env = 0,
    .require_formatting = 1,
    .dis_color = 0,
    .dis_timestamp = 0,
    .binary_mode = 0,
    .reserved = 0}};

constexpr esp_log_config_t kErrorLogConfig = {
  .opts = {
    .log_level = ESP_LOG_ERROR,
    .constrained_env = 0,
    .require_formatting = 1,
    .dis_color = 0,
    .dis_timestamp = 0,
    .binary_mode = 0,
    .reserved = 0}};
void initializeSystem();
void initializeWifiStack();

#endif  // COMMON_HPP