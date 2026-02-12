#ifndef CORE_HPP
#define CORE_HPP
#include "../common/global.hpp"
#include "../config_wrapper/config_wrapper.hpp"
#include "../http_wrapper/http_server_wrapper.hpp"
#include "../rotation_controller/rotation_controller.hpp"
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
    void aboba() {
        rotation_controller_.startRotation(RotationController::Motor::MOTOR_1);
        vTaskDelay(pdMS_TO_TICKS(1500));
        rotation_controller_.stopRotation(RotationController::Motor::MOTOR_1);
        static bool state = true;
        state = !state;
        if (state) {
            rotation_controller_.setDir(RotationController::Direction::CLOCKWISE);
        } else {
            rotation_controller_.setDir(
              RotationController::Direction::COUNTER_CLOCKWISE);
        }
    }

  private:
    void init();
    bool wifiRestart();
    bool startAp();
    bool startStation();
    std::optional<int> rotation_task_id_ = std::nullopt;

    void blinkCrap();
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

    void applyZoom(
      const WSPacket::PacketZoom* zoom_packet,
      [[maybe_unused]] int sockfd) {
        esp_log(
          kInfoLogConfig,
          "CORE",
          "ZOOM state changed to: %s",
          zoom_packet->state ? "ON" : "OFF");
        std::unique_ptr<WSPacket::PacketRespBase> packet =
          std::make_unique<WSPacket::PacketRespZoom>(WSPacket::RespType::OK);

        // do work here
        if (zoom_packet->state) {
            rotation_controller_.startRotation(RotationController::Motor::MOTOR_1);
        } else {
            rotation_controller_.stopRotation(RotationController::Motor::MOTOR_1);
        }
        // do work here
        http_server_.sendPacket(packet.get(), sockfd);
    }
    void applyFocus(
      const WSPacket::PacketFocus* focus_packet,
      [[maybe_unused]] int sockfd) {
        esp_log(
          kInfoLogConfig,
          "CORE",
          "FOCUS state changed to: %s",
          focus_packet->state ? "ON" : "OFF");
        std::unique_ptr<WSPacket::PacketRespBase> packet =
          std::make_unique<WSPacket::PacketRespFocus>(WSPacket::RespType::OK);

        // do work here
        if (focus_packet->state) {
            rotation_controller_.startRotation(RotationController::Motor::MOTOR_2);
        } else {
            rotation_controller_.stopRotation(RotationController::Motor::MOTOR_2);
        }
        // do work here
        http_server_.sendPacket(packet.get(), sockfd);
    }

    void applyChangeDirection(
      const WSPacket::PacketChangeDirection* direction_packet,
      [[maybe_unused]] int sockfd) {
        esp_log(
          kInfoLogConfig,
          "CORE",
          "FOCUS state changed to: %s",
          direction_packet->state ? "ON" : "OFF");
        std::unique_ptr<WSPacket::PacketRespBase> packet =
          std::make_unique<WSPacket::PacketRespFocus>(WSPacket::RespType::OK);

        // do work here
        if (direction_packet->state) {
            rotation_controller_.setDir(RotationController::Direction::CLOCKWISE);
        } else {
            rotation_controller_.setDir(
              RotationController::Direction::COUNTER_CLOCKWISE);
        }
        // do work here
        http_server_.sendPacket(packet.get(), sockfd);
    }
    std::unique_ptr<Wifi::Base> wifi_module_;
    RotationController rotation_controller_;
    HttpServerWrapper http_server_;

    inline static Core* instance_ = nullptr;
    inline static constexpr TagType TAG = "CORE";

    ConfigWrapper config_wrapper_;
};
#endif