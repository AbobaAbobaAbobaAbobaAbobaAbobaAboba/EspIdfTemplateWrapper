#ifndef I2S_WRAPPER_HPP
#define I2S_WRAPPER_HPP
#include "../gpio_common.hpp"
#include "driver/i2s.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include <atomic>
namespace GPIO {

class I2sWrapper : public GpioBase {
  public:
    static std::shared_ptr<I2sWrapper> createPin(
      gpio_num_t pin,
      uint32_t sample_rate,
      i2s_slot_mode_t mode,
      i2s_data_bit_width_t bits_per_sample);

    template <
      typename T,
      std::enable_if_t<
        std::disjunction_v<std::is_same<T, uint32_t>, std::is_same<T, int>>,
        int> = 0>
    esp_err_t write(T ticks) {
        return ESP_FAIL;
    }

    esp_err_t stop();

    [[nodiscard]] bool isConfigured() const;

    ~I2sWrapper();

  private:
    friend class std::weak_ptr<I2sWrapper>;
    friend class std::shared_ptr<I2sWrapper>;

    I2sWrapper(
      gpio_num_t pin,
      uint32_t sample_rate,
      i2s_slot_mode_t mode,
      i2s_data_bit_width_t bits_per_sample);

    void configurePin();

    std::atomic<bool> is_configured_ = false;
    i2s_std_config_t std_cfg_;
    i2s_chan_config_t channel_cfg_;
    i2s_chan_handle_t tx_handle_;
    i2s_config_t av;
    inline static constexpr TagType TAG = "I2S_WRAPPER";
};

};  // namespace GPIO
#endif