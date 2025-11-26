#ifndef STATION_WRAPPER_HPP
#define STATION_WRAPPER_HPP
#include "../common/common.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "wifi_base.hpp"
#include "wifi_group_wrapper.hpp"
#include <atomic>
#include <string>
namespace Wifi {

class StationWrapper : public Base {
  public:
    StationWrapper() { init(); }
    ~StationWrapper() {}

    // Delete copy and move operations
    StationWrapper(const StationWrapper& other) = delete;
    StationWrapper& operator=(const StationWrapper& other) = delete;
    StationWrapper(StationWrapper&& other) = delete;
    StationWrapper& operator=(StationWrapper&& other) = delete;

    bool connect(
      const std::string& ssid,
      const std::string& password,
      int max_attempts = std::numeric_limits<int>::max());

    bool waitForConnected();

    void disconnect();
    bool isConnected() const;
    std::string getIp();

    virtual void init() final;

  private:
    static void event_handler(
      void* arg,
      esp_event_base_t event_base,
      int32_t event_id,
      void* event_data);

    void resetAttempts();
    void updateConnectionState(ConnectionState new_state);

    void prepareWifiCredentials(const std::string& ssid, const std::string& password);
    bool prepareWifi();

    void handleWifiStart();
    void handleWifiDisconnect();
    void handleWifiConnected();
    void handleGotIp(ip_event_got_ip_t* event_data);

    void resetIpString();

    std::atomic<ConnectionState> connection_state_ = ConnectionState::DISCONNECTED;

    int current_attempt_ = 0;
    int max_attempts_;
    std::string ip_str;
    wifi_config_t wifi_config_ = {};
    static constexpr TagType TAG = "wifi_station";
};
};      // namespace Wifi
#endif  // wifi_station_HPP