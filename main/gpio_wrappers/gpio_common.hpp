#ifndef GPIO_COMMON_HPP
#define GPIO_COMMON_HPP
#include "../common/common.hpp"
#include <driver/gpio.h>
#include <array>
#include <memory>
#include <mutex>
enum class PullMode {
    NONE,
    PULL_UP,
    PULL_DOWN
};
enum class PinType : uint8_t {
    ABSTRACT = 0,
    BASIC,
    MCPWM
};
class GpioBase {
  public:
    static constexpr size_t kPinCount = CONFIG_SOC_GPIO_PIN_COUNT;
    GpioBase() {}
    virtual ~GpioBase() = 0;
    PinType getType() { return type_; }

  protected:
    // static void holdPin(gpio_num_t pin) {

    // }
    // static void releasePin(gpio_num_t pin) {

    // }
    static bool expired(gpio_num_t pin) {
        std::lock_guard<std::mutex> lock(mutex_);
        return gpio_instances_[pin].expired();
    }
    static void createPin(gpio_num_t pin) {}

  private:
    PinType type_;
    inline static std::mutex mutex_;
    inline static std::array<std::weak_ptr<GpioBase>, kPinCount> gpio_instances_ = {};
};
#endif