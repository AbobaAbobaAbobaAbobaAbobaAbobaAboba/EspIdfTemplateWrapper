#ifndef CORE_HPP
#define CORE_HPP
#include "../common/global.hpp"
#include "../config_wrapper/config_wrapper.hpp"
#include "../http_wrapper/http_server_wrapper.hpp"
#include "../rotation_controller/rotation_controller.hpp"
#include "../scheduler/scheduler.hpp"
#include "../sntp_wrapper/sntp_wrapper.hpp"
#include "../wifi_wrapper/wifi_handler.hpp"
#include <functional>
#include <type_traits>
class Core {
  public:
    Core() = delete;
    Core(const Credentials& _creds);
    ~Core() { instance_ = nullptr; }
    Core(const Core& other) = delete;
    Core& operator=(const Core& other) = delete;
    Core(Core&& other) = delete;
    Core& operator=(Core&& other) = delete;

  private:
    void init();
    void blinkCrap();
    static void applyPacket(
      std::shared_ptr<WSPacket::PacketBase> packet,
      [[maybe_unused]] int sockfd);

    void applyLED(const WSPacket::PacketLED* led_packet, [[maybe_unused]] int sockfd);
    void applyServo(
      const WSPacket::PacketServo* servo_packet,
      [[maybe_unused]] int sockfd);

    void applyTaskStart(
      const WSPacket::PacketTaskStart* packet,
      [[maybe_unused]] int sockfd);

    void applyTaskStop(
      const WSPacket::PacketTaskStop* packet,
      [[maybe_unused]] int sockfd);

    WifiHandler wifi_handler_;
    HttpServerWrapper http_server_;

    inline static Core* instance_ = nullptr;
    inline static constexpr TagType TAG = "CORE";
    Credentials creds_;

    ConfigWrapper wrapper;
    RotationController rotation_controller_;
};
#endif