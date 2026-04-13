#include "core.hpp"
#include "../scheduler/scheduler.hpp"
#include "soc/soc_caps.h"
Core::Core() : config_wrapper_("/settings.ini") {
    if (instance_ != nullptr) {
        throw std::runtime_error("Core instance already exists!");
    }
    instance_ = this;

    init();
}

bool isTimeInBracket(
  int startHour,
  int startMin,
  int startSec,
  int endHour,
  int endMin,
  int endSec) {
    time_t now = time(nullptr);
    tm now_time = *localtime(&now);

    // Convert current time to seconds since midnight
    int nowSeconds = now_time.tm_hour * 3600 + now_time.tm_min * 60 + now_time.tm_sec;
    int startSeconds = startHour * 3600 + startMin * 60 + startSec;
    int endSeconds = endHour * 3600 + endMin * 60 + endSec;

    if (startSeconds <= endSeconds) {
        // Normal interval (doesn't cross midnight)
        return nowSeconds >= startSeconds && nowSeconds < endSeconds;
    } else {
        // Interval crosses midnight
        return nowSeconds >= startSeconds || nowSeconds < endSeconds;
    }
}

void Core::init() {
    uv_pin = GPIO::GpioBasicWrapper::createPin(
      GPIO_NUM_18,
      GPIO_MODE_OUTPUT,
      GPIO::PullMode::PULL_DOWN);

    config_wrapper_.load();
    config_wrapper_.printContents();
    wifiRestart();
}

bool Core::wifiRestart() {
    esp_log(kInfoLogConfig, TAG, "Restarting WiFi 0...");
    if(http_server_) {
        forceRestartEsp();
    }
    http_server_.reset();
    esp_log(kInfoLogConfig, TAG, "Restarting WiFi 1...");
    if (!startStation()) {
        if (!startAp()) {
            throw std::runtime_error(
              "Failed to start WiFi in both Station and AP modes");
            return false;
        } else {
          esp_log(kInfoLogConfig, TAG, "Restarting WiFi 2...");
        }
    }
    esp_log(kInfoLogConfig, TAG, "Restarting WiFi 3...");
    http_server_ = std::make_unique<HttpServerWrapper>();
    http_server_->startWebserver();
    http_server_->registerCallback(&Core::applyPacket);
    wifi_module_->setTxPower(78);
    return true;
}

bool Core::startAp() {
    wifi_module_.reset();
    wifi_module_ = std::make_unique<Wifi::ApWrapper>();
    auto ptr = static_cast<Wifi::ApWrapper*>(wifi_module_.get());

    std::string ssid =
      config_wrapper_.get(std::string("ap"), std::string("ssid"), std::string(""));
    std::string password =
      config_wrapper_.get(std::string("ap"), std::string("password"), std::string(""));

    esp_log(kInfoLogConfig, TAG, "Restarting WiFi 4 %s %s", ssid.c_str(), password.c_str());
    if (ssid.empty() || password.empty()) {
        esp_log(kWarnLogConfig, TAG, "Ap SSID or password is empty");
        return false;
    }

    return ptr->startAP(ssid, password);
}

bool Core::startStation() {
    wifi_module_.reset();
    wifi_module_ = std::make_unique<Wifi::StationWrapper>();
    auto ptr = static_cast<Wifi::StationWrapper*>(wifi_module_.get());

    std::string ssid =
      config_wrapper_.get(std::string("station"), std::string("ssid"), std::string(""));
    std::string password = config_wrapper_.get(
      std::string("station"),
      std::string("password"),
      std::string(""));
    esp_log(kWarnLogConfig, TAG, "Station SSID: %s", ssid.c_str());
    esp_log(kWarnLogConfig, TAG, "Station password: %s", password.c_str());
    if (ssid.empty() || password.empty()) {
        esp_log(kWarnLogConfig, TAG, "Station SSID or password is empty");
        return false;
    }
    ptr->connect(ssid, password, kMaxConnectionStationAttempts);

    while(ptr->getConnectionState() == Wifi::ConnectionState::CONNECTING) {
      esp_log(kWarnLogConfig, TAG, "while loop");
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    esp_log(kWarnLogConfig, TAG, "after while loop");
    return ptr->getConnectionState() == Wifi::ConnectionState::CONNECTED;
  }

void Core::applyPacket(
  std::shared_ptr<WSPacket::PacketBase> packet,
  [[maybe_unused]] int sockfd) {
    static_assert(
      4 == static_cast<int>(WSPacket::PacketType::COUNT),
      "PacketType::COUNT mismatch. Check if enough Packet structs exist");

    esp_log(
      kErrorLogConfig,
      "CORE",
      "Received packet: %d",
      static_cast<int>(packet->packet_type));

    switch (static_cast<WSPacket::PacketType>(packet->packet_type)) {
        case WSPacket::PacketType::LED: {
            WSPacket::PacketLED* led_packet =
              static_cast<WSPacket::PacketLED*>(packet.get());
            instance_->applyLED(led_packet, sockfd);

            break;
        }
        case WSPacket::PacketType::CREDENTIALS: {
            WSPacket::PacketCredentials* led_packet =
              static_cast<WSPacket::PacketCredentials*>(packet.get());
            instance_->applyCredentials(led_packet, sockfd);

            break;
        }
        case WSPacket::PacketType::UV_GET_STATUS: {
            WSPacket::PacketUvIsActive* uv_is_active_packet =
              static_cast<WSPacket::PacketUvIsActive*>(packet.get());
            instance_->applyUvIsActive(uv_is_active_packet, sockfd);
            break;
        }
        case WSPacket::PacketType::UV_SET_JOB: {
            WSPacket::PacketUvSetJob* uv_is_active_packet =
              static_cast<WSPacket::PacketUvSetJob*>(packet.get());
            instance_->applyUvSetJob(uv_is_active_packet, sockfd);

            break;
        }
        default:
            esp_log(
              kErrorLogConfig,
              "CORE",
              "Received unknown packet type: %d",
              static_cast<int>(packet->packet_type));
            break;
    }
}

void Core::applyLED(
  const WSPacket::PacketLED* led_packet,
  [[maybe_unused]] int sockfd) {
    esp_log(
      kInfoLogConfig,
      "CORE",
      "LED state changed to: %s",
      led_packet->state ? "ON" : "OFF");
    std::unique_ptr<WSPacket::PacketRespBase> packet =
      std::make_unique<WSPacket::PacketRespLED>(WSPacket::RespType::OK);

    if (!(instance_->http_server_)) {
        esp_log(kErrorLogConfig, "CORE", "HTTP server is not initialized!");
        return;
    }
    instance_->http_server_->sendPacket(packet.get(), sockfd);
}

void Core::applyUvIsActive(
  const WSPacket::PacketUvIsActive* packet,
  [[maybe_unused]] int sockfd) {
    bool is_valid = true;
    std::optional<uint64_t> res;
    if (uv_task_id_.has_value()) {
        res = Scheduler::Scheduler::getRemainingTime(uv_task_id_.value());

    } else {
        is_valid = false;
    }

    if (!res.has_value()) {
        is_valid = false;
    }
    if (is_valid) {
        int aboba = res.value();
        esp_log(kErrorLogConfig, TAG, "aboba rem time %d!", aboba);
        std::unique_ptr<WSPacket::PacketRespBase> packet_resp =
          std::make_unique<WSPacket::PacketRespUvIsActive>(
            res.value(),
            1,
            WSPacket ::RespType::OK);
        if (!(instance_->http_server_)) {
            esp_log(kErrorLogConfig, "CORE", "HTTP server is not initialized!");
            return;
        }
        if (!(instance_->http_server_)) {
            esp_log(kErrorLogConfig, "CORE", "HTTP server is not initialized!");
            return;
        }
        instance_->http_server_->sendPacket(packet_resp.get(), sockfd);
        return;

    } else {
        std::unique_ptr<WSPacket::PacketRespBase> packet_resp =
          std::make_unique<WSPacket::PacketRespUvIsActive>(
            0,
            0,
            WSPacket ::RespType::OK);
        if (!(instance_->http_server_)) {
            esp_log(kErrorLogConfig, "CORE", "HTTP server is not initialized!");
            return;
        }
        instance_->http_server_->sendPacket(packet_resp.get(), sockfd);
        return;
    }
}

void Core::applyUvSetJob(
  const WSPacket::PacketUvSetJob* packet,
  [[maybe_unused]] int sockfd) {
    esp_log(
      kInfoLogConfig,
      "CORE",
      "Setting UV job to %f sec %d ",
      packet->duration,
      packet->state);

    WSPacket::RespType result = WSPacket::RespType::OK;

    if (uv_task_id_.has_value()) {
        Scheduler::Scheduler::forceExecute(uv_task_id_.value());
        Scheduler::Scheduler::unschedule(uv_task_id_.value());
        result = WSPacket::RespType::WARNING;
        uv_task_id_ = std::nullopt;
    }

    if (packet->state) {
        Scheduler::Status task_status;
        int task_id;

        std::tie(task_status, task_id) =
          Scheduler::Scheduler::schedule(TimeConv::sec(packet->duration), true, [&]() {
              this->uv_pin->write(false);
              ;
          });

        if (task_status == Scheduler::Status::ERROR) {
            esp_log(kErrorLogConfig, "CORE", "Failed to schedule UV job");
            Scheduler::Scheduler::unschedule(task_id);
            result = WSPacket::RespType::ERROR;
        } else {
            uv_task_id_ = task_id;
        }
        uv_pin->write(packet->state);
    }

    std::unique_ptr<WSPacket::PacketRespBase> packet_resp =
      std::make_unique<WSPacket::PacketRespUvSetJob>(result);
    if (!(instance_->http_server_)) {
        esp_log(kErrorLogConfig, "CORE", "HTTP server is not initialized!");
        return;
    }
    instance_->http_server_->sendPacket(packet_resp.get(), sockfd);
}