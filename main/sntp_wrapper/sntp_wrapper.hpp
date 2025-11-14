#ifndef SNTP_WRAPPER_HPP
#define SNTP_WRAPPER_HPP
#include "../common/common.hpp"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include <chrono>
#include <ctime>
#include <format>
#include <string>
constexpr uint32_t kSyncInterval = 6UL * 60UL * 60UL * 1000UL;

struct Time {
    uint8_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    std::string toString() {
        return std::format(
          "Time {:02d}:{:02d}:{:02d}. Date {:02d}.{:02d}.{:04d}",
          hour,
          minute,
          second,
          day,
          month,
          year + 1900);
    }
};

class SntpWrapper {
  public:
    SntpWrapper() = delete;
    SntpWrapper(const SntpWrapper& _other) = delete;
    SntpWrapper(SntpWrapper&& _other) = delete;
    SntpWrapper& operator=(const SntpWrapper& _other) = delete;
    SntpWrapper& operator=(SntpWrapper&& _other) = delete;

    void stop() { esp_netif_sntp_deinit(); }

    // Check if time synchronization is complete
    static bool isSynced() {
        return esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
    }
    static bool isInit() { return is_init_; }
    // Wait for time synchronization with a timeout
    esp_err_t syncWait(std::chrono::milliseconds timeout) {
        return esp_netif_sntp_sync_wait(pdMS_TO_TICKS(timeout.count()));
    }

    // Get the current system time (use after synchronization)
    static std::chrono::system_clock::time_point get_current_time() {
        return std::chrono::system_clock::now();
    }

    static Time getCurrentTime() {
        auto now = get_current_time();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm* tm_info = std::localtime(&time_t_now);
        Time t;
        t.year = static_cast<uint8_t>(tm_info->tm_year);
        t.month = static_cast<uint8_t>(tm_info->tm_mon + 1);
        t.day = static_cast<uint8_t>(tm_info->tm_mday);
        t.hour = static_cast<uint8_t>(tm_info->tm_hour);
        t.minute = static_cast<uint8_t>(tm_info->tm_min);
        t.second = static_cast<uint8_t>(tm_info->tm_sec);
        return t;
    }

  private:
    friend class Core;
    static void start() {
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("0.ru.pool.ntp.org");

        esp_sntp_set_sync_interval(kSyncInterval);

        // Add more servers
        config.servers[1] = "1.ru.pool.ntp.org";
        config.servers[2] = "2.ru.pool.ntp.org";
        config.num_of_servers = 3;
        setenv("TZ", "MSK-3", 1);
        config.sync_cb = time_sync_notification_cb;

        esp_netif_sntp_init(&config);

        while (!SntpWrapper::isSynced()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        is_init_ = true;
    }
    static void time_sync_notification_cb(struct timeval* tv) {
        std::time_t sec = tv->tv_sec;
        std::tm* tm_info = std::localtime(&sec);
        char time_str[64];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        esp_log(kInfoLogConfig, TAG, "Time is synced! %s", time_str);
    }

    inline static bool is_init_ = false;
    inline static constexpr TagType TAG = "SNTP_WRAPPER";
};
#endif