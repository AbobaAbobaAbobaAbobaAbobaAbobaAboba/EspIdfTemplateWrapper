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
constexpr uint64_t day_in_microseconds = 24 * 60 * 60 * 1000000ULL;
constexpr uint64_t day_in_seconds = 24 * 60 * 60;

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
    esp_err_t startSynced(uint64_t microseconds);

    esp_err_t continueSynced();
    esp_err_t stop();
    esp_err_t deleteTimer();

    std::optional<uint64_t> getRemainingTime() const;

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

    static std::optional<uint64_t> getRemainingTime(int id);

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

    inline static std::recursive_mutex mutex_;
    inline static std::atomic<bool> is_mutex_locked_ = false;
    inline static std::atomic<int> id_counter_ = 0;
    inline static std::unordered_map<int, Task> task_map_ = {};

    inline static QueueHandle_t id_queue_ = xQueueCreate(kQueueSize, sizeof(int));

    inline static std::atomic<bool> is_init_ = false;
};
};  // namespace Scheduler
#endif