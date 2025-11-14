#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "../common/common.hpp"
#include "../sntp_wrapper/sntp_wrapper.hpp"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <atomic>
#include <cmath>
#include <forward_list>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <utility>
namespace Scheduler {
constexpr uint64_t day_in_microseconds = 1 * 1 * 60 * 1 * 1000000ULL;
constexpr uint64_t day_in_seconds = /*24 * 60 * 60*/ 30 * 60;  // 05hrs //86400;

enum class Status : uint8_t {
    PENDING = 0,
    ERROR,
    INVALID
};

enum class TaskType : uint8_t {
    TIMEOUT,
    ALARM,
    SINGLESHOT,
    SYNCED,
    INVALID
};
// aboba look here
//я создал таймер на 18:03
//  по идее он должен работать раз в час, т.е 19 03. 20 03, и тж
//  и создал таймер на следующие сутки, на 18:00, т.е он должен
//  выстрелить 1 раз в 18:30 и потом каждые 60 минут, т.е 18:30, 19:30 20:30
struct Task {
    /// @brief
    /// if ERROR is fired
    /// owner of the task has to clear it manually
    /// otherwise it will hang there forever

    Task();

    Task(const Task& other) = delete;

    Task& operator=(const Task& other) = delete;

    Task(Task&& other) noexcept;
    Task& operator=(Task&& other) noexcept;

    ~Task();

    esp_err_t createTimer(const esp_timer_create_args_t& timer_cfg);
    esp_err_t startOnce(uint64_t microseconds);
    esp_err_t startPeriodic(uint64_t microseconds);
    esp_err_t startSynced(uint64_t microseconds) {
        if (!SntpWrapper::isInit()) {
#ifdef ENABLE_SAFENET
            throw std::runtime_error(
              "Attempting to create timed task when global time is not init");
#elif
            esp_log(
              kErrorLogConfig,
              TAG,
              "Attempting to create timed task when global time is not init");
#endif
        }
        sync_period_ = microseconds;

        timeval tv;
        gettimeofday(&tv, nullptr);
        uint64_t now_micros =
          static_cast<uint64_t>(tv.tv_sec) * 1000000ULL + tv.tv_usec;

        expected_time_ = std::make_unique<uint64_t>(now_micros + sync_period_);
        return startOnce(microseconds);
    }

    esp_err_t continueSynced() {
        if (!expected_time_) {
            throw std::runtime_error(
              "attempting continue synced when timer is not a sync one!");
        }

        struct timeval tv;
        gettimeofday(&tv, nullptr);
        uint64_t now_micros =
          static_cast<uint64_t>(tv.tv_sec) * 1000000ULL + tv.tv_usec;

        const int64_t diff_micros =
          static_cast<int64_t>(now_micros) - static_cast<int64_t>(*expected_time_);

        *expected_time_ += sync_period_;

        const uint64_t abs_diff_micros = std::abs(diff_micros);

        esp_log(
          kInfoLogConfig,
          TAG,
          "The difference was %lld %llu microseconds",
          diff_micros,
          abs_diff_micros);

        if (abs_diff_micros < 1'000'000) {  // Example threshold: 1ms; adjust as needed
                                            // (original was ~1s)
            return startOnce(sync_period_);
        }

        int64_t adjusted_period = static_cast<int64_t>(sync_period_) - diff_micros;
        if (adjusted_period < 1) {
            adjusted_period = 1;  // Clamp to minimum to avoid invalid timer periods
        }

        return startOnce(static_cast<uint64_t>(adjusted_period));
    }
    esp_err_t stop();
    esp_err_t deleteTimer();

    std::string name_;

    std::unique_ptr<int> id_;
    esp_timer_handle_t timer_ = nullptr;

    Status status_ = Status::ERROR;
    TaskType task_type_;

    std::function<void()> cb_;
    std::function<void(esp_err_t)> error_cb_ = nullptr;

    std::optional<uint64_t> getPeriod();

  private:
    std::unique_ptr<uint64_t> expected_time_ = nullptr;
    uint64_t sync_period_ = 0;
    inline static constexpr TagType TAG = "TASK";
};

class Scheduler {
  public:
    Scheduler() = delete;
    Scheduler(const Scheduler& other) = delete;
    Scheduler& operator=(const Scheduler& other) = delete;
    Scheduler(Scheduler&& other) = delete;
    Scheduler& operator=(Scheduler&& other) = delete;

    static void init();
    static Status getStatus(int id) { return {}; }

    static std::tuple<Status, int> schedule(
      uint64_t microseconds,
      bool is_singleshot,
      const std::function<void()>& execute_callback,
      const std::function<void(esp_err_t)>& error_callback = nullptr,
      const std::string& name = "");

    static std::tuple<Status, int> schedule_at(
      int hour,
      int minute,
      int second,
      bool is_singleshot,
      const std::function<void()>& execute_callback,
      const std::function<void(esp_err_t)>& error_callback,
      const std::string& name = "");

    static bool unschedule(int id);
    static bool forceExecute(int id) { return innerExecute(id); }

  private:
    static void handler(void* arg);
    static bool innerExecute(int id);

    static std::tuple<Status, int> innerSchedule(
      uint64_t microseconds,
      TaskType task_type,
      const std::function<void()>& execute_callback,
      const std::function<void(esp_err_t)>& error_callback,
      const std::string& name);

    static void worker([[maybe_unused]] void* pvParameters);

    inline static constexpr int kQueueSize = 128;
    inline static constexpr TagType TAG = "SCHEDULER";

    inline static std::mutex mutex_;
    inline static std::atomic<bool> is_mutex_locked_ = false;
    inline static std::atomic<int> id_counter_ = 0;
    inline static std::unordered_map<int, Task> task_map_ = {};

    inline static QueueHandle_t id_queue_ = xQueueCreate(kQueueSize, sizeof(int));

    inline static std::atomic<bool> is_init_ = false;
};
};  // namespace Scheduler
#endif