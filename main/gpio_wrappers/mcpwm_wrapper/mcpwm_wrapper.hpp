#ifndef MCPWM_WRAPPER_HPP
#define MCPWM_WRAPPER_HPP
#include "../gpio_common.hpp"
#include "driver/mcpwm_cap.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_timer.h"
#include <atomic>
namespace GPIO {
class McpwmWrapper : public GpioBase {
  public:
    static std::shared_ptr<McpwmWrapper>
    createPin(gpio_num_t pin, uint32_t period_ticks, uint32_t resolution = 1'000'000);

    esp_err_t write(uint32_t ticks);
    esp_err_t stop(auto stop_mode = MCPWM_TIMER_START_STOP_EMPTY) {
        esp_err_t ret = mcpwm_timer_start_stop(timer_, stop_mode);

#ifdef ENABLE_SAFENET
        if (ret != ESP_OK) {
            throw std::runtime_error(
              "Failed to stop timer: " + std::string(esp_err_to_name(ret)));
        }
#endif

        return ret;
    }
    [[nodiscard]] bool isConfigured() const;
    [[nodiscard]] std::tuple<
      mcpwm_timer_config_t,
      mcpwm_operator_config_t,
      mcpwm_comparator_config_t,
      mcpwm_generator_config_t>
    getConfiguration();

    ~McpwmWrapper();

  private:
    friend class std::weak_ptr<McpwmWrapper>;
    friend class std::shared_ptr<McpwmWrapper>;

    McpwmWrapper(gpio_num_t pin, uint32_t period_ticks, uint32_t resolution);

    void configurePin(uint32_t period_ticks, uint32_t resolution);

    mcpwm_timer_config_t timer_cfg_ = {};
    mcpwm_operator_config_t oper_cfg_ = {};
    mcpwm_comparator_config_t cmp_cfg_ = {};
    mcpwm_generator_config_t gen_cfg_ = {};
    std::atomic<bool> is_configured_ = false;

    mcpwm_timer_handle_t timer_;
    mcpwm_oper_handle_t oper_;
    mcpwm_cmpr_handle_t cmp_;
    mcpwm_gen_handle_t gen_;
    inline static std::atomic<bool> timer_stopped_ = false;
};
};  // namespace GPIO
#endif