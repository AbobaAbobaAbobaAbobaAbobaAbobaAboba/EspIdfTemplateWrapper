#include "core.hpp"
Core::Core(const Credentials& _creds) : creds_(_creds), wrapper("/settings.ini") {
    if (instance_ != nullptr) {
        throw std::runtime_error("Core instance already exists!");
    }
    instance_ = this;
    init();
}
void Core::init() {
    wifi_handler_.connect(creds_.ssid, creds_.password, 20);
    wifi_handler_.waitForConnected();
    esp_log(
      kErrorLogConfig,
      "ABOBA",
      "Time before %s ",
      sntp_wrapper_.getCurrentTime().toString().c_str());

    if (wifi_handler_.isConnected()) {
        sntp_wrapper_.start();
    }

    wrapper.load();
    wrapper.printContents();

    while (!sntp_wrapper_.is_synced()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    esp_log(
      kErrorLogConfig,
      "ABOBA",
      "Time after %s ",
      sntp_wrapper_.getCurrentTime().toString().c_str());

    http_server_.startWebserver();
    http_server_.registerCallback(&Core::applyPacket);
}
void Core::applyPacket(
  std::shared_ptr<WSPacket::PacketBase> packet,
  [[maybe_unused]] int sockfd) {
    switch (static_cast<WSPacket::PacketType>(packet->packet_type)) {
        case WSPacket::PacketType::LED: {
            WSPacket::PacketLED* led_packet =
              static_cast<WSPacket::PacketLED*>(packet.get());
            instance_->applyLED(led_packet, sockfd);
            break;
        }
        case WSPacket::PacketType::SERVO: {
            WSPacket::PacketServo* servo_packet =
              static_cast<WSPacket::PacketServo*>(packet.get());
            instance_->applyServo(servo_packet, sockfd);
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
    global::led_pin->write(led_packet->state);

    esp_log(
      kInfoLogConfig,
      "CORE",
      "LED state changed to: %s",
      led_packet->state ? "ON" : "OFF");
    std::unique_ptr<WSPacket::PacketRespBase> packet =
      std::make_unique<WSPacket::PacketRespLED>(WSPacket::RespType::OK);
    http_server_.sendPacket(packet.get(), sockfd);
}

void Core::applyServo(
  const WSPacket::PacketServo* servo_packet,
  [[maybe_unused]] int sockfd) {
    constexpr static int angle = (2500 - 500) / 180;

    global::pwm_pin->write(500 + servo_packet->value * angle);

    esp_log(kInfoLogConfig, "CORE", "Servo value changed to: %d", servo_packet->value);
    std::unique_ptr<WSPacket::PacketRespBase> packet =
      std::make_unique<WSPacket::PacketRespServo>(WSPacket::RespType::OK);
    http_server_.sendPacket(packet.get(), sockfd);
}
