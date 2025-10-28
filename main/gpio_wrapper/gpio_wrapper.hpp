#ifndef GPIO_WRAPPER_HPP
#define GPIO_WRAPPER_HPP

#include <driver/gpio.h>
#include <esp_log.h>
#include <array>
#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
namespace GPIO {

static constexpr size_t kPinCount = 40;

enum class PullMode {
    NONE,
    PULL_UP,
    PULL_DOWN
};

class GpioWrapper {
  public:
    static std::shared_ptr<GpioWrapper> getInstance(gpio_num_t pin);

    esp_err_t write(int level);
    int read();
    [[nodiscard]] bool isConfigured() const;
    [[nodiscard]] gpio_config_t getConfiguration();
    void configure(gpio_mode_t mode, PullMode pull_mode);
    ~GpioWrapper();

  private:
    friend class std::weak_ptr<GpioWrapper>;
    friend class std::shared_ptr<GpioWrapper>;
    friend class std::array<std::shared_ptr<GpioWrapper>, kPinCount>;

    GpioWrapper(gpio_num_t pin) : pin_(pin){};
    GpioWrapper(gpio_num_t pin, gpio_mode_t mode, PullMode pull_mode = PullMode::NONE);

    void configurePin(gpio_mode_t mode, PullMode pull_mode);

    static std::array<std::weak_ptr<GpioWrapper>, kPinCount> gpio_instances_;

    inline static std::atomic<uint8_t> gpio_instances_count_ = 0;
    inline static std::recursive_mutex mutex_;

    gpio_config_t io_conf_ = {};
    std::atomic<bool> is_configured_ = false;
    gpio_num_t pin_;
};
}  // namespace GPIO

#endif  // GPIO_WRAPPER_HPP