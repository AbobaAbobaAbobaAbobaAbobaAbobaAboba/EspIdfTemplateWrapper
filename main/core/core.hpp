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
    void sleep(gpio_num_t pin) {
        wifi_module_.reset();
        esp_deep_sleep_enable_gpio_wakeup(1ULL << pin, ESP_GPIO_WAKEUP_GPIO_HIGH);
        esp_deep_sleep_start();
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
    std::unique_ptr<Wifi::Base> wifi_module_;
    HttpServerWrapper http_server_;

    inline static Core* instance_ = nullptr;
    inline static constexpr TagType TAG = "CORE";

    ConfigWrapper config_wrapper_;
};
#endif