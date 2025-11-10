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

enum class Status : uint8_t {
    PENDING = 0,
    ERROR
};
struct Task {
    /// @brief
    /// if ERROR is fired
    /// owner of the task has to clear it manually
    /// otherwise it will hang there forever

    Task() {
#ifdef ENABLE_SAFENET
        esp_log(kErrorLogConfig, TAG, "TASK CREATED");
#endif
    }

    Task(const Task& other) = delete;
    Task(Task&& other) = default;

    Task& operator=(const Task& other) = delete;
    Task& operator=(Task&& other) = default;

    ~Task() {
        if (timer_) {
            esp_timer_delete(timer_);
        }
#ifdef ENABLE_SAFENET
        if (id_) {
            esp_log(kErrorLogConfig, TAG, "TASK REMOVED %d", *id_);
        } else {
            esp_log(kErrorLogConfig, TAG, "TASK REMOVED WITH EMPTY ID");
        }
#endif
    }
    std::unique_ptr<int> id_;
    std::string name_;
    esp_timer_handle_t timer_ = nullptr;
    Status status_ = Status::ERROR;
    std::function<void()> cb_;
    bool is_singleshot_ = true;

    inline static constexpr TagType TAG = "TASK";
};
class Scheduler {
  public:
    Scheduler() = delete;
    Scheduler(const Scheduler& other) = delete;
    Scheduler& operator=(const Scheduler& other) = delete;
    Scheduler(Scheduler&& other) = delete;
    Scheduler& operator=(Scheduler&& other) = delete;

    static void init() {
        if (is_init_) return;

        xTaskCreate(&Scheduler::worker, "worker", 4096, nullptr, 5, nullptr);
    }
    static Status getStatus(int id) { return {}; }

    static std::tuple<Status, int> schedule(
      uint64_t microseconds,
      bool is_singleshot,
      const std::function<void()>& callback,
      const std::string& name = "");

    static bool unschedule(int id);

  private:
    static void handler(void* arg) {
        int id = *static_cast<int*>(arg);

        esp_log(kInfoLogConfig, TAG, "Timer fired! Sending ID to worker: %d", id);

        // Send ID to worker queue (from ISR or timer callback is safe)
        BaseType_t higher_priority_task_woken = pdFALSE;
        xQueueSendFromISR(id_queue_, &id, &higher_priority_task_woken);
    }

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