#include "scheduler.hpp"
namespace Scheduler {
std::tuple<Status, int> Scheduler::Scheduler::schedule(
  uint64_t microseconds,
  bool is_singleshot,
  const std::function<void()>& callback,
  const std::string& name) {
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
    task.cb_ = callback;
    task.is_singleshot_ = is_singleshot;

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
    if (is_singleshot) {
        start_err = esp_timer_start_once(task.timer_, microseconds);
    } else {
        start_err = esp_timer_start_periodic(task.timer_, microseconds);
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

void Scheduler::Scheduler::worker([[maybe_unused]] void* pvParameters) {
    int id;

    esp_log(kInfoLogConfig, TAG, "Scheduler worker task has been created!");

    for (;;) {
        // Block until something arrives in the queue
        if (xQueueReceive(id_queue_, &id, portMAX_DELAY) == pdTRUE) {
            std::lock_guard<std::mutex> guard(mutex_);
            esp_log(kInfoLogConfig, TAG, "Worker has started working on %d!", id);

            auto it = task_map_.find(id);
            if (it == task_map_.end()) {
                esp_log(kErrorLogConfig, TAG, "INVALID ID %d!", id);
                continue;
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
            if (it->second.is_singleshot_) {
                task_map_.erase(it);
            }
            esp_log(kInfoLogConfig, TAG, "Worker has finished working on %d!", id);
            esp_log(kErrorLogConfig, TAG, "MAP SIZE %d!", task_map_.size());
        }
    }
}
};  // namespace Scheduler