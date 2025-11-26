#include "ap_wrapper.hpp"

namespace Wifi {

bool ApWrapper::startAP(
  const std::string& ssid,
  const std::string& password,
  uint8_t channel,
  wifi_auth_mode_t authmode,
  uint8_t max_connections) {
    prepareCredentials(ssid, password, channel, authmode, max_connections);

    esp_log(
      kInfoLogConfig,
      "WIFI_HANDLER",
      "Starting AP with SSID %s, password %s, channel %d",
      wifi_config_.ap.ssid,
      wifi_config_.ap.password,
      channel);

    return prepareWifi();
}

std::string ApWrapper::getIp() {
    return ip_str;
}

void ApWrapper::event_handler(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data) {
    ApWrapper* handler = static_cast<ApWrapper*>(arg);
    esp_log(
      kInfoLogConfig,
      "wifi_station",
      "Received event %s %d  \n",
      event_base,
      event_id);

    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_AP_START) {
            handler->handleAPStart();
        } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
            auto* event = static_cast<wifi_event_ap_staconnected_t*>(event_data);
            handler->handleAPStationConnected(event);
        } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            auto* event = static_cast<wifi_event_ap_stadisconnected_t*>(event_data);
            handler->handleAPStationDisconnected(event);
        }
    }
}

};  // namespace Wifi