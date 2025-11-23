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
    void applyRotationFromConfigAboba();

  private:
    void init();

    std::optional<int> rotation_task_id_ = std::nullopt;
    void scheduleRotation(
      int _steps,
      int _interval,
      RotationController::Direction _dir,
      RotationController::StepMode _step_mode) {
        rotation_controller_.setDir(_dir);
        rotation_controller_.setStepMode(_step_mode);

        esp_log(
          kInfoLogConfig,
          "ABOBA",
          "Scheduling rotation with %d %d %d %d",
          _steps,
          _interval,
          static_cast<int>(_dir),
          static_cast<int>(_step_mode));

        if (rotation_task_id_.has_value()) {
            esp_log(
              kErrorLogConfig,
              TAG,
              "attempting to schedule rotation when previous is still running! "
              "Unscheduling....");
            unscheduleRotation();
        }

        auto callback = [_steps, this]() { rotation_controller_.rotate(_steps); };
        auto [status, id] =
          Scheduler::Scheduler::schedule(_interval, false, callback, nullptr);
        if (status != Scheduler::Status::PENDING) {
            throw std::runtime_error("Failed to schedule task in scheduleRotation");
        }
        rotation_task_id_ = id;
    }
    void unscheduleRotation() {
        if (!rotation_task_id_.has_value()) {
            throw std::runtime_error("Attempting to unschedule an unscheduled task!");
        }
        bool status = Scheduler::Scheduler::unschedule(rotation_task_id_.value());

        if (!status) {
            throw std::runtime_error(
              "Attempting to unschedule a task resulted in error!");
        }
        rotation_task_id_.reset();
    }

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
            if (!Scheduler::Scheduler::unschedule(rotation_task_id_.value())) {
                esp_log(kErrorLogConfig, TAG, "Failed to unschedule!");
            }
            rotation_task_id_.reset();
        }

        rotation_controller_.setDir(rs_packet->direction);
        rotation_controller_.setStepMode(rs_packet->step_mode);

        auto [status, id] = Scheduler::Scheduler::schedule(
          rs_packet->interval_microseconds,
          false,
          [this, steps = rs_packet->steps]() { rotation_controller_.rotate(steps); });
        rotation_task_id_ = id;
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

    Credentials creds_;

    ConfigWrapper config_wrapper_;
    RotationController rotation_controller_;
};
#endif