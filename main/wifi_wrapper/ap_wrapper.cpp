#include "ap_wrapper.hpp"
#include "dhcpserver/dhcpserver.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include <socket.h>
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

void ApWrapper::init() {
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

void ApWrapper::setCustomIp() {
    // Stop DHCP server on AP interface (we want static IP)
    ESP_ERROR_CHECK(
      esp_netif_dhcps_stop(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF")));

    esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0, sizeof(ap_ip_info));

    // 192.168.0.1  netmask 255.255.255.0  gateway = same as IP
    IP4_ADDR(&ap_ip_info.ip, 192, 168, 0, 1);
    IP4_ADDR(&ap_ip_info.netmask, 255, 255, 255, 0);
    IP4_ADDR(&ap_ip_info.gw, 192, 168, 0, 1);

    esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ap_ip_info));

    // Restart DHCP server so connected stations get IPs from 192.168.0.x range
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));

    ESP_LOGI(TAG, "Custom AP IP set to 192.168.0.1");
}
bool ApWrapper::prepareWifi() {
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
    setCustomIp();
    setTxPower(78);
    return true;
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

void ApWrapper::prepareCredentials(
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

void ApWrapper::handleAPStart() {
    esp_log(kInfoLogConfig, "WIFI_HANDLER", "AP started successfully");

    // Optional: You can fetch and log the AP IP here (default is 192.168.4.1)
    // esp_netif_ip_info_t ip_info;
    // esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"),
    // &ip_info); Then convert to string similar to handleGotIp()
}

void ApWrapper::handleAPStationConnected(wifi_event_ap_staconnected_t* event_data) {
    esp_log(
      kInfoLogConfig,
      "WIFI_HANDLER",
      "Station " MACSTR " connected, AID=%d",
      MAC2STR(event_data->mac),
      event_data->aid);
}

void ApWrapper::handleAPStationDisconnected(
  wifi_event_ap_stadisconnected_t* event_data) {
    esp_log(
      kInfoLogConfig,
      "WIFI_HANDLER",
      "Station " MACSTR " disconnected, AID=%d",
      MAC2STR(event_data->mac),
      event_data->aid);
}

};  // namespace Wifi