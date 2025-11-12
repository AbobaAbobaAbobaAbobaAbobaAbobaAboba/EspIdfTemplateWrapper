#include "dac_wrapper.hpp"

namespace GPIO {
// Constructor: Initialize GPIO with specified mode and optional pull resistors

DacWrapper::DacWrapper(gpio_num_t pin) : GpioBase(pin, PinType::DAC) {
    static_assert(std::is_constant_evaluated(), "pin must be compile-time constant");

    dac_channel_mask_t mask;
    if (pin == GPIO_NUM_25) {
        mask = DAC_CHANNEL_MASK_CH0;
    } else if (pin == GPIO_NUM_26) {
        mask = DAC_CHANNEL_MASK_CH1;
    } else {
        throw std::runtime_error("Attempting to assign non-dac pin");
        return;
    }
    cont_cfg = {
      .chan_mask = mask,
      .desc_num = 4,
      .buf_size = 2048,
      .freq_hz = 44100,
      .offset = 0,
      .clk_src = DAC_DIGI_CLK_SRC_APLL,  // Using APLL as clock source to get a wider
                                         // frequency range
      /* Assume the data in buffer is 'A B C D E F'
       * DAC_CHANNEL_MODE_SIMUL:
       *      - channel 0: A B C D E F
       *      - channel 1: A B C D E F
       * DAC_CHANNEL_MODE_ALTER:
       *      - channel 0: A C E
       *      - channel 1: B D F
       */
      .chan_mode = DAC_CHANNEL_MODE_SIMUL,
    };

    configurePin();
}  // namespace GPIO

std::shared_ptr<DacWrapper> DacWrapper::createPin(gpio_num_t pin) {
    if (!expired(pin)) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error("Trying to create a non-expired pin");
#elif
        return {};
#endif
    }

    esp_log(kErrorLogConfig, TAG, "Creating new I2S pin %d", pin);

    std::lock_guard<std::recursive_mutex> lock(GpioBase::mutex_);

    auto ptr_stump = std::shared_ptr<GpioBase>(new DacWrapper(pin));

    insertPin(ptr_stump);
    return std::static_pointer_cast<DacWrapper>(ptr_stump);
}

esp_err_t DacWrapper::stop() {
    return ESP_FAIL;
}

[[nodiscard]] bool DacWrapper::isConfigured() const {
    return is_configured_;
}

DacWrapper::~DacWrapper() {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        esp_log(kErrorLogConfig, "GPIO_WRAPPER", "Invalid GPIO pin: %d", pin_);
    }
#endif

    esp_log(kInfoLogConfig, "GPIO_WRAPPER", "GPIO %d reset", pin_);
}

void DacWrapper::configurePin() {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        throw std::invalid_argument("Invalid GPIO pin: " + std::to_string(pin_));
    }
#endif

    esp_err_t err = dac_continuous_new_channels(&cont_cfg, &dac_handle);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to set dac new channel");
    }
#endif

    err = dac_continuous_enable(dac_handle);
#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to enable dac");
    }
#endif

    is_configured_ = true;
}
};  // namespace GPIO