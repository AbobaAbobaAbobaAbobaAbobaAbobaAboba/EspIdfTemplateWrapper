#ifndef MCPWM_WRAPPER_HPP
#define MCPWM_WRAPPER_HPP
#include "../gpio_common.hpp"

namespace GPIO {
class McpwmWrapper {
  public:
    static std::shared_ptr<McpwmWrapper> getInstance(gpio_num_t pin);

    esp_err_t write(int level);
    int read();

    [[nodiscard]] bool isConfigured() const;
    [[nodiscard]] gpio_config_t getConfiguration();

    void configure(gpio_mode_t mode, PullMode pull_mode);

    ~McpwmWrapper();

  private:
    friend class std::weak_ptr<McpwmWrapper>;
    friend class std::shared_ptr<McpwmWrapper>;

    McpwmWrapper(gpio_num_t pin) : pin_(pin){};
    McpwmWrapper(gpio_num_t pin, gpio_mode_t mode, PullMode pull_mode = PullMode::NONE);

    void configurePin(gpio_mode_t mode, PullMode pull_mode);

    gpio_config_t io_conf_ = {};
    std::atomic<bool> is_configured_ = false;
    gpio_num_t pin_;
}
};  // namespace GPIO
#endif