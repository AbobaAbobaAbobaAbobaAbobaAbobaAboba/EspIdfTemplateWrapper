#ifndef CORE_HPP
#define CORE_HPP
#include "../http_wrapper/http_server_wrapper.hpp"
#include "../wifi_wrapper/wifi_handler.hpp"
#include <functional>
class Core {
  public:
    Core() {
        if (instance_ != nullptr) {
            throw std::runtime_error("Core instance already exists!");
        }
        instance_ = this;
        init();
    }
    ~Core() { instance_ = nullptr; }
    Core(const Core& other) = delete;
    Core& operator=(const Core& other) = delete;
    Core(Core&& other) = delete;
    Core& operator=(Core&& other) = delete;

  private:
    void init() {
        wifi_handler_.connect("kgsn2g", "kbtaPR45ka$", 20);
        wifi_handler_.waitForConnected();

        http_server_.startWebserver();
        http_server_.registerCallback(&Core::applyPacket);
    }
    static void applyPacket(
      std::shared_ptr<WSPacket::PacketBase> packet,
      [[maybe_unused]] int sockfd) {
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
    void applyLED(const WSPacket::PacketLED* led_packet, [[maybe_unused]] int sockfd) {
        led->write(led_packet->state);

        esp_log(
          kInfoLogConfig,
          "CORE",
          "LED state changed to: %s",
          led_packet->state ? "ON" : "OFF");
        std::unique_ptr<WSPacket::PacketRespBase> packet =
          std::make_unique<WSPacket::PacketRespLED>(WSPacket::RespType::OK);
        http_server_.sendPacket(packet.get(), sockfd);
    }
    WifiHandler wifi_handler_;
    HttpServerWrapper http_server_;
    inline static Core* instance_ = nullptr;
};
#endif