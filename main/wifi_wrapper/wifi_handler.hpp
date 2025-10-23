#ifndef WIFI_HANDLER_HPP
#define WIFI_HANDLER_HPP
#include "../common/common.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "wifi_group_wrapper.hpp"
#include <atomic>
#include <string>

enum class ConnectionState : uint8_t {
    CONNECTED,
    CONNECTING,
    DISCONNECTED,
    FAILED_TO_CONNECT
};
class WifiHandler {
  public:
    WifiHandler() {
#ifdef ENABLE_SAFENET
        if (exists_) {
            throw std::runtime_error("WifiHandler already exists!");
        }
#endif
        init();
    }
    ~WifiHandler() {
    }

    // Delete copy and move operations
    WifiHandler(const WifiHandler& other) = delete;
    WifiHandler& operator=(const WifiHandler& other) = delete;
    WifiHandler(WifiHandler&& other) = delete;
    WifiHandler& operator=(WifiHandler&& other) = delete;

    bool
    connect(const std::string& ssid, const std::string& password, int max_attempts = 5);
    bool waitForConnected();

    void disconnect();
    bool isConnected() const;
    std::string getIp() {
        return ip_str;
    }

  private:
    static void event_handler(
      void* arg,
      esp_event_base_t event_base,
      int32_t event_id,
      void* event_data);

    void init();

    void resetAttempts();
    void updateConnectionState(ConnectionState new_state);

    void prepareWifiCredentials(const std::string& ssid, const std::string& password);
    bool prepareWifi();

    void handleWifiStart();
    void handleWifiDisconnect();
    void handleWifiConnected();
    void handleGotIp(ip_event_got_ip_t* event_data);

    void resetIpString();
#if ENABLE_SAFENET
    inline static std::atomic<bool> exists_ = false;
#endif

    std::atomic<ConnectionState> connection_state_ = ConnectionState::DISCONNECTED;
    static const char* TAG;
    int current_attempt_ = 0;
    int max_attempts_ = 5;
    std::string ip_str;
    wifi_config_t wifi_config_ = {};
};
#endif  // WIFI_HANDLER_HPP