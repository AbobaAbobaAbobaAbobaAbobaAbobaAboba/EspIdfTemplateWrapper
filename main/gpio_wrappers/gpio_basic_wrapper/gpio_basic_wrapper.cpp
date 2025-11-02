#include "gpio_basic_wrapper.hpp"
namespace GPIO {
// Constructor: Initialize GPIO with specified mode and optional pull resistors
GpioBasicWrapper::GpioBasicWrapper(gpio_num_t pin, gpio_mode_t mode, PullMode pull_mode)
    : GpioBase(pin, PinType::BASIC) {
    configurePin(mode, pull_mode);
}

// Destructor: Reset GPIO to safe state
GpioBasicWrapper::~GpioBasicWrapper() {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        esp_log(kErrorLogConfig, "GPIO_WRAPPER", "Invalid GPIO pin: %d", pin_);
    }
#endif
    gpio_reset_pin(pin_);
    esp_log(kInfoLogConfig, "GPIO_WRAPPER", "GPIO %d reset", pin_);
}

std::shared_ptr<GpioBasicWrapper>
GpioBasicWrapper::createPin(gpio_num_t pin, gpio_mode_t mode, PullMode pull_mode) {
    if (!expired(pin)) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error("Trying to create a non-expired pin");
#elif
        return {};
#endif
    }
    esp_log(kErrorLogConfig, "GPIO_WRAPPER", "Creating new BASIC pin %d", pin);

    std::lock_guard<std::recursive_mutex> lock(GpioBase::mutex_);
    auto ptr_stump =
      std::shared_ptr<GpioBase>(new GpioBasicWrapper(pin, mode, pull_mode));
    insertPin(ptr_stump);
    return std::static_pointer_cast<GpioBasicWrapper>(ptr_stump);
}

// Write value to GPIO (HIGH=1, LOW=0)
esp_err_t GpioBasicWrapper::write(int level) {
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
    esp_err_t ret = gpio_set_level(pin_, level);
#ifdef ENABLE_SAFENET
    if (ret != ESP_OK) {
        throw std::runtime_error(
          "Failed to write to GPIO " + std::to_string(pin_) + ": "
          + esp_err_to_name(ret));
    }
#endif
    return ret;
}

// Read value from GPIO (returns 0 or 1)
int GpioBasicWrapper::read() {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        throw std::invalid_argument("Invalid GPIO pin: " + std::to_string(pin_));
        return -1;
    }
    if (!is_configured_) {
        throw std::runtime_error(
          "GPIO pin " + std::to_string(pin_) + " is not configured.");
        return -1;
    }
#endif
    return gpio_get_level(pin_);
}

[[nodiscard]] bool GpioBasicWrapper::isConfigured() const {
    return is_configured_;
}

[[nodiscard]] gpio_config_t GpioBasicWrapper::getConfiguration() {
    return io_conf_;
}

void GpioBasicWrapper::configurePin(gpio_mode_t mode, PullMode pull_mode) {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        throw std::invalid_argument("Invalid GPIO pin: " + std::to_string(pin_));
    }
#endif

    // Configure GPIO
    io_conf_ = {};
    io_conf_.intr_type = GPIO_INTR_DISABLE;  // No interrupts
    io_conf_.mode = mode;                    // Input or Output mode
    io_conf_.pin_bit_mask = (1ULL << pin_);  // Select pin

    switch (pull_mode) {
        case PullMode::NONE:
            io_conf_.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf_.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case PullMode::PULL_UP:
            io_conf_.pull_up_en = GPIO_PULLUP_ENABLE;
            io_conf_.pull_down_en = GPIO_PULLDOWN_DISABLE;
            break;
        case PullMode::PULL_DOWN:
            io_conf_.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf_.pull_down_en = GPIO_PULLDOWN_ENABLE;
            break;
    }

    esp_err_t ret = gpio_config(&io_conf_);
#ifdef ENABLE_SAFENET
    if (ret != ESP_OK) {
        throw std::runtime_error(
          "Failed to configure GPIO " + std::to_string(pin_) + ": "
          + esp_err_to_name(ret));
    }
#endif
    is_configured_ = true;
    esp_log(
      kInfoLogConfig,
      "GPIO_WRAPPER",
      "GPIO %d initialized in %s mode",
      pin_,
      mode == GPIO_MODE_OUTPUT ? "output" : "input");
}
};