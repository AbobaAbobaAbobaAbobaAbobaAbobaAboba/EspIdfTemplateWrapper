#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include "../common/common.hpp"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <atomic>
#include <forward_list>
#include <functional>
#include <memory>
#include <mutex>
#include <tuple>
namespace Scheduler {
constexpr uint64_t day_in_microseconds = /*24 * 60 * 60 **/ 1 * 1000000ULL;
enum class Status : uint8_t {
    PENDING = 0,
    ERROR
};

enum class TaskType : uint8_t {
    TIMEOUT,
    ALARM,
    SINGLESHOT
};

struct Task {
    /// @brief
    /// if ERROR is fired
    /// owner of the task has to clear it manually
    /// otherwise it will hang there forever

    Task();

    Task(const Task& other) = delete;
    Task(Task&& other) = default;

    Task& operator=(const Task& other) = delete;
    Task& operator=(Task&& other) = default;

    ~Task();
    std::unique_ptr<int> id_;
    std::string name_;
    esp_timer_handle_t timer_ = nullptr;
    Status status_ = Status::ERROR;
    std::function<void()> cb_;
    std::function<void(esp_err_t)> error_cb_ = nullptr;
    TaskType task_type_;

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
      const std::string& name);

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