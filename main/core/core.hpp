#ifndef CORE_HPP
#define CORE_HPP
#include "../common/global.hpp"
#include "../config_wrapper/config_wrapper.hpp"
#include "../http_wrapper/http_server_wrapper.hpp"
#include "../scheduler/scheduler.hpp"
#include "../sntp_wrapper/sntp_wrapper.hpp"
#include "../wifi_wrapper/ap_wrapper.hpp"
#include "../wifi_wrapper/station_wrapper.hpp"
#include <esp_sleep.h>
#include <functional>
#include <string>
#include <type_traits>
class Core {
  public:
    Core();
    ~Core() { instance_ = nullptr; }

    Core(const Core& other) = delete;
    Core& operator=(const Core& other) = delete;
    Core(Core&& other) = delete;
    Core& operator=(Core&& other) = delete;
    void sleep(gpio_num_t pin) { wifi_module_.reset(); }

  private:
    void init();
    bool wifiRestart();
    bool startAp();
    bool startStation();
    std::optional<int> rotation_task_id_ = std::nullopt;

    static void applyPacket(
      std::shared_ptr<WSPacket::PacketBase> packet,
      [[maybe_unused]] int sockfd);

    void applyLED(const WSPacket::PacketLED* packet, [[maybe_unused]] int sockfd);
    void applyCredentials(
      const WSPacket::PacketCredentials* packet,
      [[maybe_unused]] int sockfd) {
        config_wrapper_.set(std::string("station"), std::string("ssid"), packet->ssid);
        config_wrapper_.set(
          std::string("station"),
          std::string("password"),
          packet->password);

        config_wrapper_.save();
        config_wrapper_.load();

        std::unique_ptr<WSPacket::PacketRespBase> packet_resp =
          std::make_unique<WSPacket::PacketRespCredentials>(WSPacket::RespType::OK);

        http_server_.sendPacket(packet_resp.get(), sockfd);

        wifiRestart();
    }

    void applyUvIsActive(
      const WSPacket::PacketUvIsActive* packet,
      [[maybe_unused]] int sockfd) {
        bool is_valid = true;
        if (!uv_task_id_.has_value()) {
            is_valid = false;
        }
        auto res = Scheduler::Scheduler::getRemainingTime(uv_task_id_.value());
        if (!res.has_value()) {
            is_valid = false;
        }
        if (is_valid) {
            std::unique_ptr<WSPacket::PacketRespBase> packet_resp =
              std::make_unique<WSPacket::PacketRespUvIsActive>(
                res.value(),
                1,
                WSPacket ::RespType::OK);
            instance_->http_server_.sendPacket(packet_resp.get(), sockfd);
            return;

        } else {
            std::unique_ptr<WSPacket::PacketRespBase> packet_resp =
              std::make_unique<WSPacket::PacketRespUvIsActive>(
                0,
                0,
                WSPacket ::RespType::OK);
            instance_->http_server_.sendPacket(packet_resp.get(), sockfd);
            return;
        }
    }

    void applyUvSetJob(
      const WSPacket::PacketUvSetJob* packet,
      [[maybe_unused]] int sockfd) {
        esp_log(
          kInfoLogConfig,
          "CORE",
          "Setting UV job to %d %f sec",
          packet->duration,
          packet->state);

        uv_pin->write(packet->state);
        WSPacket::RespType result = WSPacket::RespType::OK;
        if (packet->state) {
            Scheduler::Status task_status;
            int task_id;

            std::tie(task_status, task_id) = Scheduler::Scheduler::schedule(
              TimeConv::sec(packet->duration),
              true,
              [&]() {
                  this->uv_pin->write(false);
                  ;
              });
            if (uv_task_id_.has_value()) {
                Scheduler::Scheduler::forceExecute(uv_task_id_.value());
                Scheduler::Scheduler::unschedule(uv_task_id_.value());
                result = WSPacket::RespType::WARNING;
                uv_task_id_ = std::nullopt;
            }

            if (task_status == Scheduler::Status::ERROR) {
                esp_log(kErrorLogConfig, "CORE", "Failed to schedule UV job");
                Scheduler::Scheduler::unschedule(task_id);
                result = WSPacket::RespType::ERROR;
            } else {
                uv_task_id_ = task_id;
            }
        }
        std::unique_ptr<WSPacket::PacketRespBase> packet_resp =
          std::make_unique<WSPacket::PacketRespUvSetJob>(result);
        instance_->http_server_.sendPacket(packet_resp.get(), sockfd);
    }

    std::unique_ptr<Wifi::Base> wifi_module_;
    std::shared_ptr<GPIO::GpioBasicWrapper> uv_pin;
    HttpServerWrapper http_server_;
    std::optional<int> uv_task_id_ = std::nullopt;

    inline static Core* instance_ = nullptr;
    inline static constexpr TagType TAG = "CORE";

    ConfigWrapper config_wrapper_;
};
#endif