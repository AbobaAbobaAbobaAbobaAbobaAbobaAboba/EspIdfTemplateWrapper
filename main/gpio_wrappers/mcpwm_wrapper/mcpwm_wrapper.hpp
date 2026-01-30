#ifdef ENABLE_MCPWM
#ifndef MCPWM_WRAPPER_HPP
#define MCPWM_WRAPPER_HPP
#include "../gpio_common.hpp"
#include "driver/mcpwm_cap.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_types.h"
#include "soc/soc_caps.h"
#include <atomic>
namespace GPIO {
#define SOC_MCPWM_TIMERS_PER_GROUP 3
#define SOC_MCPWM_GROUPS           2
class McpwmTimer {
  public:
    McpwmTimer(int group_id, uint32_t period_ticks, uint32_t resolution) {
        esp_log(kInfoLogConfig, TAG, "Values: %d %d", period_ticks, resolution);
        timer_cfg_ = {
          .group_id = group_id,
          .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,

          .resolution_hz = resolution,
          .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
          .period_ticks = period_ticks,

          .intr_priority = 0,
          .flags =
            {.update_period_on_empty = false,
             .update_period_on_sync = false,
             .allow_pd = false}

        };
        configureTimer();
    }
    ~McpwmTimer() { mcpwm_del_timer(timer_); }
    static std::shared_ptr<McpwmTimer> createTimer(
      uint32_t group_id,
      uint32_t period_ticks,
      uint32_t resolution = 1'000'000);
    static size_t size(uint32_t group_id) {
        size_t count = 0;
        for (size_t i = 0; i < SOC_MCPWM_TIMERS_PER_GROUP; ++i) {
            if (!timers_[group_id][i].expired()) {
                ++count;
            }
        }
        return count;
    }
    uint32_t getGroupId() { return timer_cfg_.group_id; }
    uint32_t getPeriodTicks() { return timer_cfg_.period_ticks; }
    uint32_t getResolution() { return timer_cfg_.resolution_hz; }

  private:
    friend class std::weak_ptr<McpwmTimer>;
    friend class std::shared_ptr<McpwmTimer>;
    friend class McpwmWrapper;
    mcpwm_timer_handle_t& handle() { return timer_; }
    void configureTimer() {
        // Configure GPIO

        esp_err_t err = mcpwm_new_timer(&timer_cfg_, &timer_);

#ifdef ENABLE_SAFENET
        if (err != ESP_OK) {
            throw std::runtime_error("Failed to configure timer ");
        }
#endif
        err = mcpwm_timer_enable(timer_);

#ifdef ENABLE_SAFENET
        if (err != ESP_OK) {
            throw std::runtime_error("Failed to enable ");
        }
#endif
    }

    mcpwm_timer_config_t timer_cfg_ = {};
    mcpwm_timer_handle_t timer_;
    inline static std::array<
      std::array<std::weak_ptr<McpwmTimer>, SOC_MCPWM_TIMERS_PER_GROUP>,
      SOC_MCPWM_GROUPS>
      timers_ = {{}};
    inline static std::mutex mutex_;
    inline static constexpr TagType TAG = "MCPWM_TIMER";
};

class McpwmWrapper : public GpioBase {
  public:
    static std::shared_ptr<McpwmWrapper> createPin(
      gpio_num_t pin,
      std::shared_ptr<McpwmTimer> timer);

    template <
      typename T,
      std::enable_if_t<
        std::disjunction_v<std::is_same<T, uint32_t>, std::is_same<T, int>>,
        int> = 0>
    esp_err_t write(T ticks) {
#ifdef ENABLE_SAFENET
        if (!GPIO_IS_VALID_GPIO(pin_)) {
            throw std::invalid_argument("Invalid GPIO pin: " + std::to_string(pin_));
            return ESP_ERR_INVALID_ARG;
        }
        if (!is_configured_) {
            throw std::runtime_error(
              "GPIO pin " + std::to_string(pin_) + " is not configured.");
            return ESP_ERR_INVALID_STATE;
        }
#endif

        esp_err_t ret = mcpwm_comparator_set_compare_value(cmp_, ticks);

        esp_log(kInfoLogConfig, "MCPWM_PIN", "Write res: %d", ret);

#ifdef ENABLE_SAFENET
        if (ret != ESP_OK) {
            throw std::runtime_error(
              "Failed to write to GPIO " + std::to_string(pin_) + ": "
              + esp_err_to_name(ret));
        }
#endif

        ret = mcpwm_timer_start_stop(timer_->handle(), MCPWM_TIMER_START_NO_STOP);

#ifdef ENABLE_SAFENET
        if (ret != ESP_OK) {
            throw std::runtime_error(
              "Failed to start timer: " + std::string(esp_err_to_name(ret)));
        }
#endif

        return ret;
    }

    esp_err_t stop(
      mcpwm_timer_start_stop_cmd_t stop_mode = MCPWM_TIMER_START_STOP_EMPTY);

    [[nodiscard]] bool isConfigured() const;

    [[nodiscard]] std::tuple<
      std::shared_ptr<McpwmTimer>,
      mcpwm_operator_config_t,
      mcpwm_comparator_config_t,
      mcpwm_generator_config_t>
    getConfiguration();

    ~McpwmWrapper();

  private:
    friend class std::weak_ptr<McpwmWrapper>;
    friend class std::shared_ptr<McpwmWrapper>;

    McpwmWrapper(gpio_num_t pin, std::shared_ptr<McpwmTimer> timer);

    void configurePin();

    mcpwm_operator_config_t oper_cfg_ = {};
    mcpwm_comparator_config_t cmp_cfg_ = {};
    mcpwm_generator_config_t gen_cfg_ = {};
    std::atomic<bool> is_configured_ = false;

    std::shared_ptr<McpwmTimer> timer_;
    mcpwm_oper_handle_t oper_;
    mcpwm_cmpr_handle_t cmp_;
    mcpwm_gen_handle_t gen_;
    inline static std::atomic<bool> timer_stopped_ = false;
};
};  // namespace GPIO
#endif
#endif  // ENABLE_MCPWM