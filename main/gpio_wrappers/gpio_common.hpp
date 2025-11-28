#ifndef GPIO_COMMON_HPP
#define GPIO_COMMON_HPP
#include "../common/common.hpp"
#include <driver/gpio.h>
#include <array>
#include <memory>
#include <mutex>
namespace GPIO {
enum class PullMode {
    NONE,
    PULL_UP,
    PULL_DOWN
};
enum class PinType : uint8_t {
    ABSTRACT = 0,
    BASIC,
    MCPWM,
    I2S,
    DAC
};
class GpioBase {
  public:
    static constexpr size_t kPinCount = CONFIG_SOC_GPIO_PIN_COUNT;
    static std::shared_ptr<GpioBase> getInstance(gpio_num_t pin) {
#ifdef ENABLE_SAFENET
        if (!GPIO_IS_VALID_GPIO(pin)) {
            throw std::invalid_argument("Invalid GPIO pin: " + std::to_string(pin));
        }
#endif

        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return gpio_instances_[pin].lock();
    }
    virtual ~GpioBase() = default;
    PinType getType() {
        return type_;
    }
    gpio_num_t getPin() {
        return pin_;
    }

  protected:
    static bool expired(gpio_num_t pin) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return gpio_instances_[pin].expired();
    }

    static void insertPin(std::weak_ptr<GpioBase> gpio) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
#ifdef ENABLE_SAFENET
        if (gpio.expired()) {
            throw std::runtime_error("Attempting to insert invalid ptr!");
        }
#endif

        auto locked = gpio.lock();
#ifdef ENABLE_SAFENET
        if (!gpio_instances_[locked->pin_].expired()) {
            throw std::runtime_error(
              "Attempting to insert a pin at the place of an UNEXPIRED pin!");
        }
#endif
        gpio_instances_[locked->pin_] = gpio;
    }

    GpioBase(gpio_num_t pin, PinType type) : pin_(pin), type_(type) {
    }

    const gpio_num_t pin_;
    const PinType type_ = PinType::ABSTRACT;

    inline static std::recursive_mutex mutex_;

  private:
    GpioBase() = delete;
    GpioBase(gpio_num_t pin) : pin_(pin) {
    }
    friend class std::weak_ptr<GpioBase>;
    friend class std::shared_ptr<GpioBase>;

    inline static std::array<std::weak_ptr<GpioBase>, kPinCount> gpio_instances_ = {};
    inline static size_t gpio_count_ = 0;
};
};  // namespace GPIO
#endif