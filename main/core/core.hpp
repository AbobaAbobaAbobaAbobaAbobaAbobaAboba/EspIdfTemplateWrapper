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

constexpr static int kMaxConnectionStationAttempts = 5;
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
  void forceRestartEsp() {
      esp_log(kWarnLogConfig, TAG, "Forcing restart...");
      esp_wifi_stop();           // stops the WiFi stack cleanly
      esp_restart();
  }
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
          esp_log(kInfoLogConfig, TAG, "After load1");
        std::unique_ptr<WSPacket::PacketRespBase> packet_resp =
          std::make_unique<WSPacket::PacketRespCredentials>(WSPacket::RespType::OK);
          esp_log(kInfoLogConfig, TAG, "After load2");
        http_server_->sendPacket(packet_resp.get(), sockfd);
        esp_log(kInfoLogConfig, TAG, "After load3");
        wifiRestart();
        esp_log(kInfoLogConfig, TAG, "After load4");
    }

    void applyUvIsActive(
      const WSPacket::PacketUvIsActive* packet,
      [[maybe_unused]] int sockfd);

    void applyUvSetJob(
      const WSPacket::PacketUvSetJob* packet,
      [[maybe_unused]] int sockfd);

    std::unique_ptr<Wifi::Base> wifi_module_;
    std::shared_ptr<GPIO::GpioBasicWrapper> uv_pin;
    std::shared_ptr<GPIO::GpioBasicWrapper> sound_pin;
    std::unique_ptr<HttpServerWrapper> http_server_;
    std::optional<int> uv_task_id_ = std::nullopt;

    inline static Core* instance_ = nullptr;
    inline static constexpr TagType TAG = "CORE";

    ConfigWrapper config_wrapper_;
};
#endif