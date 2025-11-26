#include "core.hpp"
#include "../scheduler/scheduler.hpp"
Core::Core(const Credentials& _creds)
    : creds_(_creds),
      config_wrapper_("/settings.ini") {
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
    wifi_station_.connect(creds_.ssid, creds_.password);
    wifi_station_.setTxPower(78);  // Max power
    wifi_station_.waitForConnected();

    if (wifi_station_.isConnected()) {
        SntpWrapper::start();
    }

    config_wrapper_.load();
    config_wrapper_.printContents();

    http_server_.startWebserver();
    http_server_.registerCallback(&Core::applyPacket);
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
      1 == static_cast<int>(WSPacket::PacketType::COUNT),
      "PacketType::COUNT mismatch. Check if enough Packet structs exist");
    switch (static_cast<WSPacket::PacketType>(packet->packet_type)) {
        case WSPacket::PacketType::LED: {
            WSPacket::PacketLED* led_packet =
              static_cast<WSPacket::PacketLED*>(packet.get());
            instance_->applyLED(led_packet, sockfd);
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
