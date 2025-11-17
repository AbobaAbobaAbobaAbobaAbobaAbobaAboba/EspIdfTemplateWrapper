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

    void applyRotationStart(
      const WSPacket::PacketRotationStart* rs_packet,
      [[maybe_unused]] int sockfd) {
        if (rotation_task_id_ != std::nullopt) {
            Scheduler::Scheduler::unschedule(rotation_task_id_.value());
        }
        rotation_controller_.setDir(rs_packet->direction);
        rotation_controller_.setStepMode(rs_packet->step_mode);

        auto [status, id] = Scheduler::Scheduler::schedule(
          rs_packet->interval_microseconds,
          false,
          [this, steps = rs_packet->steps]() {
              rotation_controller_.rotate(steps);
              rotation_task_id_.reset();
          });

        if (status != Scheduler::Status::PENDING) {
            esp_log(kErrorLogConfig, "CORE", "Failed to schedule rotation %d ", status);
            Scheduler::Scheduler::unschedule(id);

            std::unique_ptr<WSPacket::PacketRespBase> packet =
              std::make_unique<WSPacket::PacketRespRotationStart>(
                WSPacket::RespType::ERROR);

            http_server_.sendPacket(packet.get(), sockfd);
            return;
        }

        esp_log(
          kInfoLogConfig,
          "CORE",
          "Rotation started with params: %d %d %d %d",
          rs_packet->interval_microseconds,
          rs_packet->steps,
          rs_packet->direction,
          rs_packet->step_mode);

        std::unique_ptr<WSPacket::PacketRespBase> packet =
          std::make_unique<WSPacket::PacketRespRotationStart>(WSPacket::RespType::OK);

        http_server_.sendPacket(packet.get(), sockfd);
    }
    void applyRotationStop(
      const WSPacket::PacketRotationStop* rs_packet,
      [[maybe_unused]] int sockfd) {
        if (rotation_task_id_ != std::nullopt) {
            Scheduler::Scheduler::unschedule(rotation_task_id_.value());
            rotation_task_id_.reset();
        }
        std::unique_ptr<WSPacket::PacketRespBase> packet =
          std::make_unique<WSPacket::PacketRespRotationStop>(WSPacket::RespType::OK);
        http_server_.sendPacket(packet.get(), sockfd);
    }

    WifiHandler wifi_handler_;
    HttpServerWrapper http_server_;

    inline static Core* instance_ = nullptr;
    inline static constexpr TagType TAG = "CORE";

    std::optional<int> rotation_task_id_ = std::nullopt;

    Credentials creds_;

    ConfigWrapper wrapper;
    RotationController rotation_controller_;
};
#endif