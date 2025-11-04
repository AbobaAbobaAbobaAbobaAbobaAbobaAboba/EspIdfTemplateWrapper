#ifndef CORE_HPP
#define CORE_HPP
#include "../common/global.hpp"
#include "../http_wrapper/http_server_wrapper.hpp"
#include "../wifi_wrapper/wifi_handler.hpp"
#include <functional>
#include <type_traits>
class Core {
  public:
    Core();
    ~Core() { instance_ = nullptr; }
    Core(const Core& other) = delete;
    Core& operator=(const Core& other) = delete;
    Core(Core&& other) = delete;
    Core& operator=(Core&& other) = delete;

  private:
    void init();

    static void applyPacket(
      std::shared_ptr<WSPacket::PacketBase> packet,
      [[maybe_unused]] int sockfd);
    void applyLED(const WSPacket::PacketLED* led_packet, [[maybe_unused]] int sockfd);
    void applyServo(
      const WSPacket::PacketServo* servo_packet,
      [[maybe_unused]] int sockfd) {
        constexpr static int angle = (2500 - 500) / 180;

        global::pwm_pin->write(500 + servo_packet->value * angle);

        esp_log(
          kInfoLogConfig,
          "CORE",
          "Servo value changed to: %d",
          servo_packet->value);
        std::unique_ptr<WSPacket::PacketRespBase> packet =
          std::make_unique<WSPacket::PacketRespServo>(WSPacket::RespType::OK);
        http_server_.sendPacket(packet.get(), sockfd);
    }
    WifiHandler wifi_handler_;
    HttpServerWrapper http_server_;

    inline static Core* instance_ = nullptr;
};
#endif