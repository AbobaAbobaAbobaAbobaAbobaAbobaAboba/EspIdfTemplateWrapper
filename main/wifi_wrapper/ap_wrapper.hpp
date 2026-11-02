#ifndef AP_WRAPPER_HPP
#define AP_WRAPPER_HPP
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

class ApWrapper : public Base {
  public:
    ApWrapper() : Base(Mode::AP) { init(); }
    ~ApWrapper() {
        esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
        esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &event_handler);
    }

    // Delete copy and move operations
    ApWrapper(const ApWrapper& other) = delete;
    ApWrapper& operator=(const ApWrapper& other) = delete;
    ApWrapper(ApWrapper&& other) = delete;
    ApWrapper& operator=(ApWrapper&& other) = delete;

    bool startAP(
      const std::string& ssid,
      const std::string& password,
      uint8_t channel = 0,
      wifi_auth_mode_t authmode = WIFI_AUTH_WPA2_PSK,
      uint8_t max_connections = 4);

    std::string getIp();

    virtual void init() final;

  protected:
    virtual bool prepareWifi() final;

  private:
    static void event_handler(
      void* arg,
      esp_event_base_t event_base,
      int32_t event_id,
      void* event_data);
    void prepareCredentials(
      const std::string& ssid,
      const std::string& password,
      uint8_t channel,
      wifi_auth_mode_t authmode,
      uint8_t max_connections);

    void aboba() { std::cout << "hui!!" << std::endl; }

    void setCustomIp();

    void handleAPStart();

    void handleAPStationConnected(wifi_event_ap_staconnected_t* event_data);

    void handleAPStationDisconnected(wifi_event_ap_stadisconnected_t* event_data);

    std::atomic<ConnectionState> connection_state_ = ConnectionState::DISCONNECTED;

    int current_attempt_ = 0;
    int max_attempts_;
    std::string ip_str;
    wifi_config_t wifi_config_ = {};
    static constexpr TagType TAG = "wifi_station";
};
};  // namespace Wifi
#endif  // wifi_station_HPP