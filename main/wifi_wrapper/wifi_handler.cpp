#include "wifi_handler.hpp"
bool WifiHandler::connect(
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
      "WIFI_HANDLER",
      "Attempting to connect to %s with pass %s \n",
      wifi_config_.sta.ssid,
      wifi_config_.sta.password);

    return prepareWifi();
}
bool WifiHandler::waitForConnected() {
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
void WifiHandler::disconnect() {
    if (connection_state_ != ConnectionState::CONNECTED) {
        esp_log(kInfoLogConfig, "WIFI_HANDLER", "Not connected to WiFi");
        return;
    }

    esp_err_t err = esp_wifi_disconnect();
    ESP_ERROR_CHECK(err);

    err = esp_wifi_stop();
    ESP_ERROR_CHECK(err);

    connection_state_ = ConnectionState::DISCONNECTED;
    esp_log(kInfoLogConfig, "WIFI_HANDLER", "Disconnected from WiFi");
}
bool WifiHandler::isConnected() const {
    return connection_state_ == ConnectionState::CONNECTED;
}
void WifiHandler::init() {
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
    exists_ = true;
}
void WifiHandler::event_handler(
  void* arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void* event_data) {
    WifiHandler* handler = static_cast<WifiHandler*>(arg);
    esp_log(
      kInfoLogConfig,
      "WIFI_HANDLER",
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
void WifiHandler::resetAttempts() {
    current_attempt_ = 0;
}
void WifiHandler::updateConnectionState(ConnectionState new_state) {
    connection_state_ = new_state;
    resetAttempts();
}
void WifiHandler::prepareWifiCredentials(
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
bool WifiHandler::prepareWifi() {
    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(err);

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config_);
    ESP_ERROR_CHECK(err);

    err = esp_wifi_start();
    ESP_ERROR_CHECK(err);

    if (err != ESP_OK) {
        esp_log(
          kInfoLogConfig,
          "WIFI_HANDLER",
          "Failed to start wifi %s \n",
          esp_err_to_name(err));

        return false;
    }
    return true;
}
void WifiHandler::handleWifiStart() {
    esp_log(kInfoLogConfig, "WIFI_HANDLER", "WiFi started, connecting to AP...");
    esp_wifi_connect();
    updateConnectionState(ConnectionState::CONNECTING);
    ++current_attempt_;
}
void WifiHandler::handleWifiDisconnect() {
    if (current_attempt_ < max_attempts_) {
        esp_wifi_connect();
        esp_log(
          kInfoLogConfig,
          "WIFI_HANDLER",
          "Reconnecting to AP, attempt %d",
          current_attempt_);
        ++current_attempt_;
    } else {
        esp_log(
          kErrorLogConfig,
          "WIFI_HANDLER",
          "Failed to connect to AP after %d attempts",
          max_attempts_);
        updateConnectionState(ConnectionState::FAILED_TO_CONNECT);
    }
}
void WifiHandler::handleWifiConnected() {
    updateConnectionState(ConnectionState::CONNECTED);
}

void WifiHandler::handleGotIp(ip_event_got_ip_t* event_data) {
    resetIpString();
    esp_ip4addr_ntoa(&event_data->ip_info.ip, ip_str.data(), ip_str.size());
    esp_log(
      kInfoLogConfig,
      "WIFI_HANDLER",
      "Connected to AP. Got IP: %s \n",
      ip_str.c_str());
    AssignedIp::ip_address = ip_str;
}

void WifiHandler::resetIpString() {
    for (auto& c : ip_str) {
        c = '\0';
    }
}
