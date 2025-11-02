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
    MCPWM
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

        // if (expired(pin)) {
        //     esp_log(kErrorLogConfig, "GPIO_WRAPPER", "Creating new pin");
        //     std::lock_guard<std::mutex> lock(mutex_);
        //     auto ptr_stump = std::shared_ptr<GpioBasicWrapper>(new GpioBase(pin));
        //     gpio_instances_[pin] = ptr_stump;
        //     return ptr_stump;
        // }

        return gpio_instances_[pin].lock();
    }
    virtual ~GpioBase() = default;
    PinType getType() {
        return type_;
    }

  protected:
    // static void holdPin(gpio_num_t pin) {

    // }
    // static void releasePin(gpio_num_t pin) {

    // }

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