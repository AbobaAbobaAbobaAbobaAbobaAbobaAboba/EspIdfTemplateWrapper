#include "i2s_wrapper.hpp"

namespace GPIO {
// Constructor: Initialize GPIO with specified mode and optional pull resistors
I2sWrapper::I2sWrapper(
  gpio_num_t pin,
  uint32_t sample_rate,
  i2s_slot_mode_t mode,
  i2s_data_bit_width_t bits_per_sample)
    : GpioBase(pin, PinType::I2S) {
    channel_cfg_ = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    std_cfg_ = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(bits_per_sample, mode),
      .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = I2S_GPIO_UNUSED,  // Internal DAC doesn't need BCLK/WS
        .ws = I2S_GPIO_UNUSED,
        .dout = I2S_GPIO_UNUSED,  // Internal DAC
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false}}};
    configurePin();
}  // namespace GPIO

std::shared_ptr<I2sWrapper> I2sWrapper::createPin(
  gpio_num_t pin,
  uint32_t sample_rate,
  i2s_slot_mode_t mode,
  i2s_data_bit_width_t bits_per_sample) {
    if (!expired(pin)) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error("Trying to create a non-expired pin");
#elif
        return {};
#endif
    }

    esp_log(kErrorLogConfig, TAG, "Creating new I2S pin %d", pin);

    std::lock_guard<std::recursive_mutex> lock(GpioBase::mutex_);

    auto ptr_stump = std::shared_ptr<GpioBase>(
      new I2sWrapper(pin, sample_rate, mode, bits_per_sample));

    insertPin(ptr_stump);
    return std::static_pointer_cast<I2sWrapper>(ptr_stump);
}

esp_err_t I2sWrapper::stop() {
    return ESP_FAIL;
}

[[nodiscard]] bool I2sWrapper::isConfigured() const {
    return is_configured_;
}

I2sWrapper::~I2sWrapper() {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        esp_log(kErrorLogConfig, "GPIO_WRAPPER", "Invalid GPIO pin: %d", pin_);
    }
#endif

    esp_log(kInfoLogConfig, "GPIO_WRAPPER", "GPIO %d reset", pin_);
}

void I2sWrapper::configurePin() {
#ifdef ENABLE_SAFENET
    if (!GPIO_IS_VALID_GPIO(pin_)) {
        throw std::invalid_argument("Invalid GPIO pin: " + std::to_string(pin_));
    }
#endif

    esp_err_t err = i2s_new_channel(&channel_cfg_, &tx_handle_, NULL);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to start i2s tx");
    }
#endif

    err = i2s_channel_init_std_mode(tx_handle_, &std_cfg_);

#ifdef ENABLE_SAFENET
    if (err != ESP_OK) {
        throw std::runtime_error("Failed to init i2s std");
    }
#endif

    i2s_dac_mode_t dac_mode = {.invert_dac = false};
    err = i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN)
#ifdef ENABLE_SAFENET
      if (err != ESP_OK) {
        throw std::runtime_error("Failed to start i2s tx");
    }
#endif

    is_configured_ = true;
}
};  // namespace GPIO