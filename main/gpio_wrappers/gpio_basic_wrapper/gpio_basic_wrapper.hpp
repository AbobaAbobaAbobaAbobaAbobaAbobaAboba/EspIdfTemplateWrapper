#ifndef GPIO_BASIC_WRAPPER_HPP
#define GPIO_BASIC_WRAPPER_HPP

#include "../gpio_common.hpp"
#include <esp_log.h>
#include <atomic>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
namespace GPIO {
class GpioBasicWrapper : public GpioBase {
  public:
    static std::shared_ptr<GpioBasicWrapper>
    createPin(gpio_num_t pin, gpio_mode_t mode, PullMode pull_mode);

    esp_err_t write(int level);
    int read();

    [[nodiscard]] bool registerIsr(const std::function<void(int)>& cb) {
        if (isr_extension_.has_value()) {
            throw std::runtime_error("ISR is already registered for this pin");
        }
        auto res = gpio_isr_handler_add(pin_, isr_handler, this);
        customErrorCheck(res, "Failed to register ISR handler");
        isr_extension_ = ISR{cb};
        return true;
    }
    void unregisterIsr() {
        if (!isr_extension_.has_value()) {
            throw std::runtime_error("ISR is not registered for this pin");
        }
        gpio_isr_handler_remove(pin_);
        isr_extension_.reset();
    }

    [[nodiscard]] bool isConfigured() const;
    [[nodiscard]] gpio_config_t getConfiguration();

    ~GpioBasicWrapper();

  private:
    static void IRAM_ATTR isr_handler(void* arg) {
        GpioBasicWrapper* gpio = static_cast<GpioBasicWrapper*>(arg);
        if (gpio->isr_extension_.has_value()) {
            if (gpio->isr_extension_->cb_) {
                gpio->isr_extension_->cb_(gpio->read());
            }
        }
    }

    friend class std::weak_ptr<GpioBasicWrapper>;
    friend class std::shared_ptr<GpioBasicWrapper>;

    GpioBasicWrapper(
      gpio_num_t pin,
      gpio_mode_t mode,
      PullMode pull_mode = PullMode::NONE);

    void configurePin(gpio_mode_t mode, PullMode pull_mode);

    gpio_config_t io_conf_ = {};
    std::atomic<bool> is_configured_ = false;
    struct ISR {
        std::function<void(int)> cb_;
    };
    std::optional<ISR> isr_extension_ = std::nullopt;
    inline static constexpr TagType TAG = "GPIO_BASIC_WRAPPER";
};
};  // namespace GPIO

  ;     // namespace GPIO
#endif  // GPIO_WRAPPER_HPP