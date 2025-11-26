#include "core.hpp"
#include "../scheduler/scheduler.hpp"
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
    config_wrapper_.load();
    config_wrapper_.printContents();
    wifiRestart();

    http_server_.startWebserver();
    http_server_.registerCallback(&Core::applyPacket);
}

bool Core::wifiRestart() {
    if (!startStation()) {
        if (!startAp()) {
            throw std::runtime_error(
              "Failed to start WiFi in both Station and AP modes");
            return false;
        }
    }
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

    return ptr->connect(ssid, password, 40);
}

void Core::blinkCrap() {
    global::led_pin->write(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    global::led_pin->write(false);
    vTaskDelay(pdMS_TO_TICKS(100));

    global::led_pin->write(true);
    vTaskDelay(pdMS_TO_TICKS(100));
    global::led_pin->write(false);
    vTaskDelay(pdMS_TO_TICKS(100));
}
void Core::applyPacket(
  std::shared_ptr<WSPacket::PacketBase> packet,
  [[maybe_unused]] int sockfd) {
    static_assert(
      2 == static_cast<int>(WSPacket::PacketType::COUNT),
      "PacketType::COUNT mismatch. Check if enough Packet structs exist");
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
    // global::led_pin->write(led_packet->state);
    Scheduler::Scheduler::schedule(TimeConv::sec(3), true, [&]() {
        this->blinkCrap();
    });
    esp_log(
      kInfoLogConfig,
      "CORE",
      "LED state changed to: %s",
      led_packet->state ? "ON" : "OFF");
    std::unique_ptr<WSPacket::PacketRespBase> packet =
      std::make_unique<WSPacket::PacketRespLED>(WSPacket::RespType::OK);
    http_server_.sendPacket(packet.get(), sockfd);
}
