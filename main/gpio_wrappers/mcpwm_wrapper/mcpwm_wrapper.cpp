#include "mcpwm_wrapper.hpp"

namespace GPIO {
// Constructor: Initialize GPIO with specified mode and optional pull resistors
McpwmWrapper::McpwmWrapper(gpio_num_t pin, std::shared_ptr<McpwmTimer> timer)
    : GpioBase(pin, PinType::MCPWM),
      timer_(timer) {
    configurePin();
}

std::shared_ptr<McpwmWrapper> McpwmWrapper::createPin(
  gpio_num_t pin,
  std::shared_ptr<McpwmTimer> timer) {
    if (!expired(pin)) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error("Trying to create a non-expired pin");
#elif
        return {};
#endif
    }

    esp_log(kErrorLogConfig, "GPIO_WRAPPER", "Creating new MCPWM pin %d", pin);

    std::lock_guard<std::recursive_mutex> lock(GpioBase::mutex_);

    auto ptr_stump = std::shared_ptr<GpioBase>(new McpwmWrapper(pin, timer));

    insertPin(ptr_stump);
    return std::static_pointer_cast<McpwmWrapper>(ptr_stump);
}

esp_err_t McpwmWrapper::stop(mcpwm_timer_start_stop_cmd_t stop_mode) {
    esp_err_t ret = mcpwm_timer_start_stop(timer_->handle(), stop_mode);

#ifdef ENABLE_SAFENET
    if (ret != ESP_OK) {
        throw std::runtime_error(
          "Failed to stop timer: " + std::string(esp_err_to_name(ret)));
    }
#endif

    return ret;
}

[[nodiscard]] bool McpwmWrapper::isConfigured() const {
    return is_configured_;
}

[[nodiscard]] std::tuple<
  std::shared_ptr<McpwmTimer>,
  mcpwm_operator_config_t,
  mcpwm_comparator_config_t,
  mcpwm_generator_config_t>
McpwmWrapper::getConfiguration() {
    return {timer_, oper_cfg_, cmp_cfg_, gen_cfg_};
}

McpwmWrapper::~McpwmWrapper() {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        esp_log(kErrorLogConfig, "GPIO_WRAPPER", "Invalid GPIO pin: %d", pin_);
    }
#endif

    esp_log(kInfoLogConfig, "GPIO_WRAPPER", "GPIO %d reset", pin_);

    mcpwm_del_operator(oper_);
    mcpwm_del_comparator(cmp_);
    mcpwm_del_generator(gen_);
}

void McpwmWrapper::configurePin() {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        throw std::invalid_argument("Invalid GPIO pin: " + std::to_string(pin_));
    }
#endif

    oper_cfg_ = {
      .group_id = 0,
      .intr_priority = 0,
      .flags =
        {.update_gen_action_on_tez = 0,
         .update_gen_action_on_tep = 0,
         .update_gen_action_on_sync = 0,
         .update_dead_time_on_tez = 0,
         .update_dead_time_on_tep = 0,
         .update_dead_time_on_sync = 0}

    };

    esp_err_t err = mcpwm_new_operator(&oper_cfg_, &oper_);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error(
          "Failed to start oper " + std::to_string(pin_) + ": " + esp_err_to_name(err));
    }
#endif

    err = mcpwm_operator_connect_timer(oper_, timer_->handle());

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error(
          "Failed to start oper " + std::to_string(pin_) + ": " + esp_err_to_name(err));
    }
#endif

    cmp_cfg_ = {
      .intr_priority = 0,
      .flags =
        {

          .update_cmp_on_tez = true,
          .update_cmp_on_tep = false,
          .update_cmp_on_sync = false

        }

    };
    err = mcpwm_new_comparator(oper_, &cmp_cfg_, &cmp_);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error(
          "Failed to start oper " + std::to_string(pin_) + ": " + esp_err_to_name(err));
    }
#endif

    gen_cfg_ = {

      .gen_gpio_num = pin_,
      .flags = {.invert_pwm = false}

    };

    err = mcpwm_new_generator(oper_, &gen_cfg_, &gen_);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error(
          "Failed to start oper " + std::to_string(pin_) + ": " + esp_err_to_name(err));
    }
#endif
    mcpwm_gen_timer_event_action_t action1 = {

      .direction = MCPWM_TIMER_DIRECTION_UP,
      .event = MCPWM_TIMER_EVENT_EMPTY,
      .action = MCPWM_GEN_ACTION_HIGH

    };
    err = mcpwm_generator_set_action_on_timer_event(gen_, action1);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error(
          "Failed to set gen action on TEZ: " + std::string(esp_err_to_name(err)));
    }
#endif

    mcpwm_gen_compare_event_action_t action2 = {
      .direction = MCPWM_TIMER_DIRECTION_UP,
      .comparator = cmp_,
      .action = MCPWM_GEN_ACTION_LOW};

    err = mcpwm_generator_set_action_on_compare_event(gen_, action2);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error(
          "Failed to set gen action on CMP: " + std::string(esp_err_to_name(err)));
    }
#endif

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error(
          "Failed to start timer: " + std::string(esp_err_to_name(err)));
    }
#endif

    is_configured_ = true;
}

std::shared_ptr<McpwmTimer>
McpwmTimer::createTimer(uint32_t group_id, uint32_t period_ticks, uint32_t resolution) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (size(group_id) >= SOC_MCPWM_TIMERS_PER_GROUP) {
#ifdef ENABLE_SAFENET
            throw std::runtime_error("Timer group has reached its maximum size");
#elif
            return {};
#endif
        }

        esp_log(
          kErrorLogConfig,
          TAG,
          "Creating new MCPWM timer with group %d",
          group_id);

        auto ptr_stump = std::shared_ptr<McpwmTimer>(
          new McpwmTimer(group_id, period_ticks, resolution));

        for (size_t i = 0; i < SOC_MCPWM_TIMERS_PER_GROUP; ++i) {
            if (timers_[group_id][i].expired()) {
                timers_[group_id][i] = ptr_stump;
            }
        }

        return std::static_pointer_cast<McpwmTimer>(ptr_stump);
    }
}

};  // namespace GPIO