#include "station_wrapper.hpp"

namespace Wifi {
bool StationWrapper::connect(
  const std::string& ssid,
  const std::string& password,
  int max_attempts) {
    // Configure WiFi
    disconnect();
    resetAttempts();
    prepareWifiCredentials(ssid, password);
    max_attempts_ = max_attempts;

    esp_log(
      kInfoLogConfig,
      "wifi_station",
      "Attempting to connect to %s with pass %s \n",
      wifi_config_.sta.ssid,
      wifi_config_.sta.password);

    return prepareWifi();
}
bool StationWrapper::waitForConnected() {
    while (true) {
        if (isConnected()) {
            return true;
        }
        if (connection_state_ == ConnectionState::FAILED_TO_CONNECT) {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
void StationWrapper::disconnect() {
    if (connection_state_ != ConnectionState::CONNECTED) {
        esp_log(kInfoLogConfig, "wifi_station", "Not connected to WiFi");
        return;
    }

    esp_err_t err = esp_wifi_disconnect();
    ESP_ERROR_CHECK(err);

    err = esp_wifi_stop();
    ESP_ERROR_CHECK(err);

    connection_state_ = ConnectionState::DISCONNECTED;
    esp_log(kInfoLogConfig, "wifi_station", "Disconnected from WiFi");
}
bool StationWrapper::isConnected() const {
    return connection_state_ == ConnectionState::CONNECTED;
}
std::string StationWrapper::getIp() {
    return ip_str;
}

void StationWrapper::init() {
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

void StationWrapper::event_handler(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data) {
    StationWrapper* handler = static_cast<StationWrapper*>(arg);
    esp_log(
      kInfoLogConfig,
      "wifi_station",
      "Received event %s %d  \n",
      event_base,
      event_id);
    // handle logic on how to reconnect for N attempts

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        handler->handleWifiStart();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        handler->handleWifiDisconnect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Handle IP address assignment
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        handler->handleWifiConnected();
        handler->handleGotIp(event);
    }
}
void StationWrapper::resetAttempts() {
    current_attempt_ = 0;
}
void StationWrapper::updateConnectionState(ConnectionState new_state) {
    connection_state_ = new_state;
    resetAttempts();
}
void StationWrapper::prepareWifiCredentials(
  const std::string& ssid,
  const std::string& password) {
    memset(&wifi_config_, 0, sizeof(wifi_config_t));
    strncpy(
      (char*)wifi_config_.sta.ssid,
      ssid.c_str(),
      sizeof(wifi_config_.sta.ssid) - 1);

    strncpy(
      (char*)wifi_config_.sta.password,
      password.c_str(),
      sizeof(wifi_config_.sta.password) - 1);

    wifi_config_.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
}
bool StationWrapper::prepareWifi() {
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        esp_log(
          kErrorLogConfig,
          "wifi_station",
          "Failed to set wifi mode %s \n",
          esp_err_to_name(err));
#ifdef ENABLE_SAFENET
        throw std::runtime_error(
          "Failed to set wifi mode: " + std::string(esp_err_to_name(err)));
#endif
        return false;
    }

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config_);
    if (err != ESP_OK) {
        esp_log(
          kErrorLogConfig,
          "wifi_station",
          "Failed to set wifi config %s \n",
          esp_err_to_name(err));
#ifdef ENABLE_SAFENET
        throw std::runtime_error(
          "Failed to set wifi config: " + std::string(esp_err_to_name(err)));
#endif
        return false;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        esp_log(
          kErrorLogConfig,
          "wifi_station",
          "Failed to start wifi  %s \n",
          esp_err_to_name(err));
#ifdef ENABLE_SAFENET
        throw std::runtime_error(
          "Failed to start wifi : " + std::string(esp_err_to_name(err)));
#endif
        return false;
    }

    return true;
}
void StationWrapper::handleWifiStart() {
    esp_log(kInfoLogConfig, "wifi_station", "WiFi started, connecting to AP...");
    esp_wifi_connect();
    updateConnectionState(ConnectionState::CONNECTING);
    ++current_attempt_;
}
void StationWrapper::handleWifiDisconnect() {
    if (current_attempt_ < max_attempts_) {
        esp_wifi_connect();
        esp_log(
          kInfoLogConfig,
          "wifi_station",
          "Reconnecting to AP, attempt %d",
          current_attempt_);
        ++current_attempt_;
    } else {
        esp_log(
          kErrorLogConfig,
          "wifi_station",
          "Failed to connect to AP after %d attempts",
          max_attempts_);
        updateConnectionState(ConnectionState::FAILED_TO_CONNECT);
    }
}
void StationWrapper::handleWifiConnected() {
    updateConnectionState(ConnectionState::CONNECTED);
}

void StationWrapper::handleGotIp(ip_event_got_ip_t* event_data) {
    resetIpString();
    esp_ip4addr_ntoa(&event_data->ip_info.ip, ip_str.data(), ip_str.size());
    esp_log(
      kInfoLogConfig,
      "wifi_station",
      "Connected to AP. Got IP: %s \n",
      ip_str.c_str());
    ip_address = ip_str;
}

void StationWrapper::resetIpString() {
    for (auto& c : ip_str) {
        c = '\0';
    }
}
};  // namespace Wifi