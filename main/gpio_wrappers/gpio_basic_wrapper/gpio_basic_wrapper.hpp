#ifndef GPIO_BASIC_WRAPPER_HPP
#define GPIO_BASIC_WRAPPER_HPP

#include "../gpio_common.hpp"
#include <esp_log.h>
#include <exception>
#include <stdexcept>
#include <string>
class GpioBasicWrapper : public GpioBase {
  public:
    static std::shared_ptr<GpioBasicWrapper> getInstance(gpio_num_t pin);

    esp_err_t write(int level);
    int read();

    [[nodiscard]] bool isConfigured() const;
    [[nodiscard]] gpio_config_t getConfiguration();

    void configure(gpio_mode_t mode, PullMode pull_mode);

    ~GpioBasicWrapper();

  private:
    friend class std::weak_ptr<GpioBasicWrapper>;
    friend class std::shared_ptr<GpioBasicWrapper>;

    GpioBasicWrapper(gpio_num_t pin) : pin_(pin){};
    GpioBasicWrapper(
      gpio_num_t pin,
      gpio_mode_t mode,
      PullMode pull_mode = PullMode::NONE);

    void configurePin(gpio_mode_t mode, PullMode pull_mode);
    gpio_num_t pin_;
    gpio_config_t io_conf_ = {};
    std::atomic<bool> is_configured_ = false;
};

#endif  // GPIO_WRAPPER_HPP