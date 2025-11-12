#include "scheduler.hpp"
namespace Scheduler {
void Scheduler::init() {
    if (is_init_) return;

    xTaskCreate(&Scheduler::worker, "worker", 4096, nullptr, 5, nullptr);
}

std::tuple<Status, int> Scheduler::schedule(
  uint64_t microseconds,
  bool is_singleshot,
  const std::function<void()>& execute_callback,
  const std::function<void(esp_err_t)>& error_callback,
  const std::string& name) {
    return innerSchedule(
      microseconds,
      is_singleshot ? TaskType::SINGLESHOT : TaskType::TIMEOUT,
      execute_callback,
      error_callback,
      name);
}

std::tuple<Status, int> Scheduler::schedule_at(
  int hour,
  int minute,
  int second,
  bool is_singleshot,
  const std::function<void()>& execute_callback,
  const std::function<void(esp_err_t)>& error_callback,
  const std::string& name) {
    if (
      hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
        throw std::invalid_argument("Invalid time values.");
    }

    time_t now = time(nullptr);
    if (now == (time_t)-1) {
        throw std::runtime_error("Failed to get current time.");
    }

    tm target = *localtime(&now);
    target.tm_hour = hour;
    target.tm_min = minute;
    target.tm_sec = second;
    target.tm_isdst = -1;  // Let mktime determine DST.

    time_t target_time = mktime(&target);
    if (target_time == (time_t)-1) {
        throw std::runtime_error("Failed to compute target time.");
    }

    if (target_time < now) {
        // Schedule for tomorrow.
        std::cout << ctime(&now) << std::endl;
        esp_log(
          kErrorLogConfig,
          TAG,
          "Current time exceeds specified one %d:%d:%d",
          target.tm_hour,
          target.tm_min,
          target.tm_sec);
        target_time += 86400;  // 24 hours in seconds.
    }

    uint64_t seconds_to_wait = static_cast<uint64_t>(target_time - now);
    uint64_t microseconds = seconds_to_wait * 1000000ULL;

    if (is_singleshot) {
        return innerSchedule(
          microseconds,
          TaskType::SINGLESHOT,
          execute_callback,
          error_callback,
          name);
    } else {
        return innerSchedule(
          microseconds,
          TaskType::ALARM,
          execute_callback,
          error_callback,
          name);
    }
}

std::tuple<Status, int> Scheduler::innerSchedule(
  uint64_t microseconds,
  TaskType task_type,
  const std::function<void()>& execute_callback,
  const std::function<void(esp_err_t)>& error_callback,
  const std::string& name = "") {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = task_map_.find(id_counter_);

    // Note that second AND will be executed only if the first one succeeds
    while (it != task_map_.end()) {
        ++id_counter_;
        it = task_map_.find(id_counter_);
    }

    Task task;
    task.id_ = std::make_unique<int>(id_counter_);
    task.name_ = "Job_" + std::to_string(id_counter_) + name;
    task.status_ = Status::PENDING;

    task.cb_ = execute_callback;
    task.error_cb_ = error_callback;
    task.task_type_ = task_type;

    esp_timer_create_args_t timer_cfg{
      .callback = &Scheduler::handler,
      .arg = task.id_.get(),
      .dispatch_method = ESP_TIMER_TASK,
      .name = task.name_.c_str(),
      .skip_unhandled_events = false};

    esp_err_t err = esp_timer_create(&timer_cfg, &task.timer_);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        task.status_ = Status::ERROR;
        throw std::runtime_error("Failed to start timer!");
    }
#endif
    task_map_[id_counter_] = std::move(task);

    esp_err_t start_err;

    switch (task.task_type_) {
        case TaskType::TIMEOUT:
            start_err = esp_timer_start_periodic(task.timer_, microseconds);
            break;
        case TaskType::ALARM:
            start_err = esp_timer_start_once(task.timer_, microseconds);
            break;
        case TaskType::SINGLESHOT:
            start_err = esp_timer_start_once(task.timer_, microseconds);
            break;
        default: {
            throw std::runtime_error("Unexpected  state in scheduler");
        }
    }

    const int res = id_counter_;
    ++id_counter_;

    if (start_err != ESP_OK) {
        return {Status::ERROR, res};
    }
    return {task.status_, res};
}

bool Scheduler::Scheduler::unschedule(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = task_map_.find(id_counter_);

    if (it != task_map_.end()) {
        task_map_.erase(it);
        return true;
    }
    return false;
}

void Scheduler::handler(void* arg) {
    int id = *static_cast<int*>(arg);

    esp_log(kInfoLogConfig, TAG, "Timer fired! Sending ID to worker: %d", id);

    // Send ID to worker queue (from ISR or timer callback is safe)
    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(id_queue_, &id, &higher_priority_task_woken);
}

bool Scheduler::innerExecute(int id) {
    esp_log(kInfoLogConfig, TAG, "Worker has started working on %d!", id);

    std::lock_guard<std::mutex> guard(mutex_);

    auto it = task_map_.find(id);
    if (it == task_map_.end()) {
        esp_log(kErrorLogConfig, TAG, "INVALID ID %d!", id);
        return false;
    }

    if (it->second.status_ != Status::PENDING) {
        esp_log(
          kErrorLogConfig,
          TAG,
          "INVALID STATUS %d!",
          static_cast<int>(it->second.status_));
        throw std::runtime_error(
          "Invalid task status was detected in scheduler while processing");
    }

    it->second.cb_();

    switch (it->second.task_type_) {
        case TaskType::ALARM: {
            esp_err_t err =
              esp_timer_start_periodic(it->second.timer_, day_in_microseconds);

            it->second.task_type_ = TaskType::TIMEOUT;

            if (err != ESP_OK) {
                it->second.status_ = Status::ERROR;
            }

            if (it->second.error_cb_) {
                it->second.error_cb_(err);
            } else {
                esp_log(kErrorLogConfig, TAG, "Error cb is not present!");
            }

            break;
        }
        case TaskType::SINGLESHOT:
            task_map_.erase(it);
            break;
        default: {
            break;
        }
    }

    return true;
}

void Scheduler::Scheduler::worker([[maybe_unused]] void* pvParameters) {
    int id;

    esp_log(kInfoLogConfig, TAG, "Scheduler worker task has been created!");

    for (;;) {
        // Block until something arrives in the queue
        if (xQueueReceive(id_queue_, &id, portMAX_DELAY) == pdTRUE) {
            innerExecute(id);
            esp_log(kInfoLogConfig, TAG, "Worker has finished working on %d!", id);
            esp_log(kErrorLogConfig, TAG, "MAP SIZE %d!", task_map_.size());
        }
    }
}

Task::Task() {
#ifdef ENABLE_SAFENET
    esp_log(kErrorLogConfig, TAG, "TASK CREATED");
#endif
}

Task::~Task() {
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
};  // namespace Scheduler