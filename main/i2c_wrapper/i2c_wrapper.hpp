#ifndef I2C_WRAPPER_HPP
#define I2C_WRAPPER_HPP

#include "../common/common.hpp"
#include <driver/i2c.h>
#include <esp_err.h>
#include <esp_log.h>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>
class I2CWrapper {
  public:
    I2CWrapper(
      i2c_port_t port,
      gpio_num_t sda_pin,
      gpio_num_t scl_pin,
      uint32_t freq_hz);

    ~I2CWrapper();

    I2CWrapper(const I2CWrapper& other) = delete;
    I2CWrapper(I2CWrapper&& other) = delete;

    I2CWrapper& operator=(const I2CWrapper& other) = delete;
    I2CWrapper& operator=(I2CWrapper&& other) = delete;

    bool isInit() { return is_init_; }

    esp_err_t write(uint8_t slave_addr, const uint8_t* data, size_t len) {
        std::vector<uint8_t> vect(data, data + len);
        return write(slave_addr, vect);
    }
    esp_err_t write(uint8_t slave_addr, const std::vector<uint8_t>& data);

    [[nodiscard]] std::tuple<esp_err_t, std::optional<std::vector<uint8_t>>> read(
      uint8_t slave_addr,
      size_t len);

  private:
    esp_err_t init();

    i2c_port_t port_;
    gpio_num_t sda_pin_;
    gpio_num_t scl_pin_;
    uint32_t freq_hz_;
    inline static bool is_init_ = false;

    inline static constexpr TagType TAG = "I2C_HANDLER";
};

#endif