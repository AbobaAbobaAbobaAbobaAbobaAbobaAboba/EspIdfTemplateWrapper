#include "core.hpp"
Core::Core() {
    if (instance_ != nullptr) {
        throw std::runtime_error("Core instance already exists!");
    }
    instance_ = this;
    init();
}
void Core::init() {
    wifi_handler_.connect("kgsn2g", "kbtaPR45ka$", 20);
    wifi_handler_.waitForConnected();

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
