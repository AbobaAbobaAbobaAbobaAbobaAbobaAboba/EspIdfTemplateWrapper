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
    ApWrapper() { init(); }
    ~ApWrapper() {}

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

    virtual void init() final {
        esp_netif_create_default_wifi_ap();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

        esp_err_t err = esp_wifi_init(&cfg);
        if (err != ESP_OK) {
            esp_log(
              kErrorLogConfig,
              TAG,
              "Failed to initialize WiFi %s \n",
              esp_err_to_name(err));
#ifdef ENABLE_SAFENET
            throw std::runtime_error(
              "Failed to initialize WiFi: " + std::string(esp_err_to_name(err)));
#endif
            deinit();
            return;
        }

        esp_event_handler_instance_register(
          WIFI_EVENT,
          ESP_EVENT_ANY_ID,
          &event_handler,
          this,
          nullptr);

        esp_event_handler_instance_register(
          IP_EVENT,
          ESP_EVENT_ANY_ID,
          &event_handler,
          this,
          nullptr);
        ip_str.resize(IP4ADDR_STRLEN_MAX + 1);
    }

  protected:
    bool prepareWifi() {
        esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);

        if (err != ESP_OK) {
            return false;
        }

        err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config_);
        if (err != ESP_OK) {
            return false;
        }

        err = esp_wifi_start();
        if (err != ESP_OK) {
            return false;
        }

        return true;
    }

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
      uint8_t max_connections) {
        memset(&wifi_config_, 0, sizeof(wifi_config_t));
        strncpy(
          (char*)wifi_config_.ap.ssid,
          ssid.c_str(),
          sizeof(wifi_config_.ap.ssid) - 1);
        wifi_config_.ap.ssid_len = strlen((char*)wifi_config_.ap.ssid);

        strncpy(
          (char*)wifi_config_.ap.password,
          password.c_str(),
          sizeof(wifi_config_.ap.password) - 1);

        wifi_config_.ap.channel = channel;
        wifi_config_.ap.authmode = authmode;
        wifi_config_.ap.max_connection = max_connections;
    }

    void handleAPStart() {
        esp_log(kInfoLogConfig, "WIFI_HANDLER", "AP started successfully");

        // Optional: You can fetch and log the AP IP here (default is 192.168.4.1)
        // esp_netif_ip_info_t ip_info;
        // esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"),
        // &ip_info); Then convert to string similar to handleGotIp()
    }

    void handleAPStationConnected(wifi_event_ap_staconnected_t* event_data) {
        esp_log(
          kInfoLogConfig,
          "WIFI_HANDLER",
          "Station " MACSTR " connected, AID=%d",
          MAC2STR(event_data->mac),
          event_data->aid);
    }

    void handleAPStationDisconnected(wifi_event_ap_stadisconnected_t* event_data) {
        esp_log(
          kInfoLogConfig,
          "WIFI_HANDLER",
          "Station " MACSTR " disconnected, AID=%d",
          MAC2STR(event_data->mac),
          event_data->aid);
    }

    std::atomic<ConnectionState> connection_state_ = ConnectionState::DISCONNECTED;

    int current_attempt_ = 0;
    int max_attempts_;
    std::string ip_str;
    wifi_config_t wifi_config_ = {};
    static constexpr TagType TAG = "wifi_station";
};
};      // namespace Wifi
#endif  // wifi_station_HPP