#ifndef CORE_HPP
#define CORE_HPP
#include "../common/global.hpp"
#include "../config_wrapper/config_wrapper.hpp"
#include "../http_wrapper/http_server_wrapper.hpp"
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
    void blinkCrap() {
        global::led_pin->write(true);
        vTaskDelay(pdMS_TO_TICKS(100));
        global::led_pin->write(false);
        vTaskDelay(pdMS_TO_TICKS(100));

        global::led_pin->write(true);
        vTaskDelay(pdMS_TO_TICKS(100));
        global::led_pin->write(false);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    static void applyPacket(
      std::shared_ptr<WSPacket::PacketBase> packet,
      [[maybe_unused]] int sockfd);

    void applyLED(const WSPacket::PacketLED* led_packet, [[maybe_unused]] int sockfd);
    void applyServo(
      const WSPacket::PacketServo* servo_packet,
      [[maybe_unused]] int sockfd);

    void applyTaskStart(
      const WSPacket::PacketTaskStart* packet,
      [[maybe_unused]] int sockfd) {
        auto exec_callback = []() {
            time_t now = time(nullptr);
            std::cout << "Fire time: " << ctime(&now) << std::endl;
        };
        auto error_callback = [](esp_err_t error) {
            std::cout << "received error: " << error << std::endl;
        };

        Scheduler::Scheduler::schedule_at(
          packet->hour,
          packet->minute,
          packet->second,
          false,
          exec_callback,
          error_callback);
        std::unique_ptr<WSPacket::PacketRespBase> resp_packet =
          std::make_unique<WSPacket::PacketRespTaskStart>(WSPacket::RespType::OK);
        http_server_.sendPacket(resp_packet.get(), sockfd);
    }

    void applyTaskStop(
      const WSPacket::PacketTaskStop* packet,
      [[maybe_unused]] int sockfd) {
        esp_log(
          kErrorLogConfig,
          TAG,
          "ABOBA UNSCHEDULED %d %d",
          packet->id,
          Scheduler::Scheduler::unschedule(packet->id));

        std::unique_ptr<WSPacket::PacketRespBase> resp_packet =
          std::make_unique<WSPacket::PacketRespTaskStart>(WSPacket::RespType::OK);
        http_server_.sendPacket(resp_packet.get(), sockfd);
    }

    WifiHandler wifi_handler_;
    HttpServerWrapper http_server_;

    inline static Core* instance_ = nullptr;
    inline static constexpr TagType TAG = "CORE";
    Credentials creds_;

    ConfigWrapper wrapper;
};
#endif