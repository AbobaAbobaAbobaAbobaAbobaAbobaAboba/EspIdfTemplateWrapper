#ifndef COMMON_HPP
#define COMMON_HPP
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "littlefs_wrapper/littlefs_wrapper.hpp"
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

inline void initializeSystem() {
    // System initialization
    std::cout << "init system is called!" << std::endl;
    esp_err_t err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    err = esp_netif_init();
    ESP_ERROR_CHECK(err);

    err = esp_event_loop_create_default();
    ESP_ERROR_CHECK(err);

    if (err != ESP_OK) {
        SystemStatus::system_status_ = false;
    } else {
        SystemStatus::system_status_ = true;
    }
    LittleFS::LittleFsWrapper::init();
    initializeWifiStack();
    // led = GPIO::GpioBasicWrapper::getInstance(LED_PIN);
    // if (led->isConfigured() == false) {
    //     led->configure(GPIO_MODE_OUTPUT, GPIO::PullMode::NONE);
    // }
}

inline void initializeWifiStack() {
    std::cout << "init wifi stack is called!" << std::endl;
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t err = esp_wifi_init(&cfg);
    ESP_ERROR_CHECK(err);

    if (err != ESP_OK) {
        SystemStatus::wifi_status_ = false;
    } else {
        SystemStatus::wifi_status_ = true;
    }
}

#endif  // COMMON_HPP