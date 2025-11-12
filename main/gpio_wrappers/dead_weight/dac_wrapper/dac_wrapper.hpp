#ifndef DAC_WRAPPER_HPP
#define DAC_WRAPPER_HPP
#include "../gpio_common.hpp"
#include "audio_example_file.h"
#include "driver/dac_continuous.h"
#include <atomic>
namespace GPIO {

// disabe DAC if ADC is required
class DacWrapper : public GpioBase {
  public:
    static std::shared_ptr<DacWrapper> createPin(gpio_num_t pin);

    esp_err_t write() {
        return dac_continuous_write(
          dac_handle,
          (uint8_t*)audio_table,
          sizeof(audio_table),
          NULL,
          -1);
    }

    esp_err_t stop();

    [[nodiscard]] bool isConfigured() const;

    ~DacWrapper();

  private:
    friend class std::weak_ptr<DacWrapper>;
    friend class std::shared_ptr<DacWrapper>;

    DacWrapper(gpio_num_t pin);

    void configurePin();

    std::atomic<bool> is_configured_ = false;

    dac_continuous_handle_t dac_handle;
    dac_continuous_config_t cont_cfg;

    inline static constexpr TagType TAG = "I2S_WRAPPER";
};

};  // namespace GPIO
#endif