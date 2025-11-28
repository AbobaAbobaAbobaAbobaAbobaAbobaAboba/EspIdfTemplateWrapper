#include "scheduler.hpp"
#include "../common/global.hpp"
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
          "Current time exceeds the specified one %d:%d:%d. Executing callback and "
          "rescheduling for tommorow",
          target.tm_hour,
          target.tm_min,
          target.tm_sec);
        target_time += day_in_seconds;  // 24 hours in seconds.
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
    std::lock_guard<std::recursive_mutex> guard(mutex_);

    auto it = task_map_.find(id_counter_);

    // Note that second AND will be executed only if the first one succeeds
    while (it != task_map_.end()) {
        esp_log(
          kInfoLogConfig,
          TAG,
          "Found an existing id, looking for an availbable one %d ",
          id_counter_.load());
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

    esp_err_t start_err = task.createTimer(timer_cfg);
#ifdef ENABLE_SAFENET
    if (start_err != ESP_OK) {
        throw std::runtime_error("Failed to start the timer");
    }
#elif
    if (start_err != ESP_OK) {
        return {Status::ERROR, res};
    }
#endif
    switch (task.task_type_) {
        case TaskType::TIMEOUT:
            start_err = task.startPeriodic(microseconds);
            break;
        case TaskType::ALARM:
            start_err = task.startOnce(microseconds);
            break;
        case TaskType::SINGLESHOT:
            start_err = task.startOnce(microseconds);
            break;
        case TaskType::SYNCED:
            start_err = task.startSynced(microseconds);
            break;
        default: {
#ifdef ENABLE_SAFENET
            throw std::runtime_error("Unexpected  state in scheduler");
#elif
            return {Status::ERROR, res};
#endif
        }
    }
    auto temp_iterator = task_map_.emplace(id_counter_, std::move(task));
    if (!temp_iterator.second) {
        throw std::runtime_error(
          "Attempting to emplace a task but the bucket was already there");
    }
    const int res = id_counter_;
    ++id_counter_;

    if (start_err != ESP_OK) {
        esp_log(
          kInfoLogConfig,
          TAG,
          "Failed to schedule %d %s ",
          microseconds,
          esp_err_to_name(start_err));
        return {Status::ERROR, res};
    }
    return {temp_iterator.first->second.status_, res};
}

bool Scheduler::Scheduler::unschedule(int id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    esp_log(kErrorLogConfig, TAG, "Attempting to unschedule %d", id);
    auto it = task_map_.find(id);

    if (it != task_map_.end()) {
        task_map_.erase(it);
        return true;
    }
    return false;
}

void Scheduler::handler(void* arg) {
    int id = *static_cast<int*>(arg);

    // Send ID to worker queue (from ISR or timer callback is safe)
    BaseType_t higher_priority_task_woken = pdFALSE;
    xQueueSendFromISR(id_queue_, &id, &higher_priority_task_woken);
}

bool Scheduler::innerExecute(int id) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);

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
            esp_err_t err = it->second.startSynced(day_in_microseconds);
            it->second.task_type_ = TaskType::SYNCED;

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
        case TaskType::SYNCED: {
            esp_err_t err = it->second.continueSynced();

            if (err != ESP_OK) {
                it->second.status_ = Status::ERROR;
            }

            if (it->second.error_cb_) {
                it->second.error_cb_(err);
            } else {
                esp_log(kErrorLogConfig, TAG, "error_cb is not present!");
            }
        }
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
            try {
                innerExecute(id);
            } catch (const std::exception& e) {
                while (true) {
                    esp_log(
                      kErrorLogConfig,
                      "EXCEPTION",
                      "%s"
                      "\n",
                      e.what());
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                };
            } catch (const char* msg) {
                while (true) {
                    esp_log(
                      kErrorLogConfig,
                      "EXCEPTION",
                      "%s"
                      "\n",
                      msg);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                };
            } catch (int code) {
                while (true) {
                    esp_log(
                      kErrorLogConfig,
                      "EXCEPTION",
                      "%d"
                      "\n",
                      code);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                };
            } catch (...) {
                while (true) {
                    esp_log(
                      kErrorLogConfig,
                      "EXCEPTION",
                      "Unknown exception occurred\n");
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    global::blinkLED(100);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                };
            }
        }
    }
}

Task::Task() {
}

Task::Task(Task&& other) noexcept
    : name_(std::move(other.name_)),
      id_(std::move(other.id_)),
      timer_(std::exchange(other.timer_, nullptr)),
      status_(std::exchange(other.status_, Status::INVALID)),
      task_type_(other.task_type_),
      cb_(std::move(other.cb_)),
      error_cb_(std::move(other.error_cb_)),
      expected_time_(std::move(other.expected_time_)) {
}

Task& Task::operator=(Task&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    // Release current timer if necessary (assuming destructor logic handles
    // deletion; call it explicitly here to free resources before moving).
    if (timer_) {
        esp_err_t err = esp_timer_stop(timer_);

        if (err != ESP_OK) {
            esp_log(
              kErrorLogConfig,
              TAG,
              "Error during the stopping of the timer! %s",
              esp_err_to_name(err));
        }

        err = esp_timer_delete(timer_);
        if (err != ESP_OK) {
            esp_log(
              kErrorLogConfig,
              TAG,
              "Error during the deletion of the timer! %s",
              esp_err_to_name(err));
        }
    }
    id_ = std::move(other.id_);
    name_ = std::move(other.name_);
    timer_ = std::exchange(other.timer_, nullptr);
    status_ = std::exchange(other.status_, Status::INVALID);
    cb_ = std::move(other.cb_);
    error_cb_ = std::move(other.error_cb_);
    task_type_ = other.task_type_;
    expected_time_ = std::move(other.expected_time_);
    return *this;
}

Task::~Task() {
    if (timer_) {
        deleteTimer();
    }
}

esp_err_t Task::createTimer(const esp_timer_create_args_t& timer_cfg) {
    esp_err_t err = esp_timer_create(&timer_cfg, &timer_);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        status_ = Status::ERROR;
        throw std::runtime_error("Failed to start timer!");
    }

#endif

    return err;
}

esp_err_t Task::startOnce(uint64_t microseconds) {
    return esp_timer_start_once(timer_, microseconds);
}

esp_err_t Task::startPeriodic(uint64_t microseconds) {
    return esp_timer_start_periodic(timer_, microseconds);
}

esp_err_t Task::startSynced(uint64_t microseconds) {
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
    uint64_t now_micros = static_cast<uint64_t>(tv.tv_sec) * 1000000ULL + tv.tv_usec;

    expected_time_ = std::make_unique<uint64_t>(now_micros + sync_period_);
    return startOnce(microseconds);
}

esp_err_t Task::continueSynced() {
    if (!expected_time_) {
        throw std::runtime_error(
          "attempting continue synced when timer is not a sync one!");
    }

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    uint64_t now_micros = static_cast<uint64_t>(tv.tv_sec) * 1000000ULL + tv.tv_usec;

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

    if (abs_diff_micros < 1'000) {  // Example threshold: 1ms; adjust as needed
        // (original was ~1s)
        esp_log(kInfoLogConfig, TAG, "Returning unchanged");
        return startOnce(sync_period_);
    }

    int64_t adjusted_period = static_cast<int64_t>(sync_period_) - diff_micros;
    if (adjusted_period < 1) {
        adjusted_period = 1;  // Clamp to minimum to avoid invalid timer periods
    }

    esp_log(kInfoLogConfig, TAG, "Adjusting to %lld", adjusted_period);
    return startOnce(static_cast<uint64_t>(adjusted_period));
}

esp_err_t Task::stop() {
    return esp_timer_stop(timer_);
}

esp_err_t Task::deleteTimer() {
    esp_err_t err = esp_timer_stop(timer_);

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        esp_log(
          kErrorLogConfig,
          TAG,
          "Error during stopping of the timer! %s",
          esp_err_to_name(err));
        return err;
    }

    err = esp_timer_delete(timer_);
    if (err != ESP_OK) {
        esp_log(
          kErrorLogConfig,
          TAG,
          "Error during deletion of the timer! %s",
          esp_err_to_name(err));
        return err;
    }
    return err;
}

std::optional<uint64_t> Task::getPeriod() {
    uint64_t period_us;
    esp_err_t err = esp_timer_get_period(timer_, &period_us);
    if (err != ESP_OK) {
        return std::nullopt;
    }
    return period_us;
}

};  // namespace Scheduler