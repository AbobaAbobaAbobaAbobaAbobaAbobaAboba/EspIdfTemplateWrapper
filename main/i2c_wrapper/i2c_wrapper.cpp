#include "i2c_wrapper.hpp"
I2CWrapper::I2CWrapper(
  i2c_port_t port,
  gpio_num_t sda_pin,
  gpio_num_t scl_pin,
  uint32_t freq_hz)
    : port_(port),
      sda_pin_(sda_pin),
      scl_pin_(scl_pin),
      freq_hz_(freq_hz) {
    if (!is_init_) {
        if (init() == ESP_OK) {
            is_init_ = true;
        }
    }
}

I2CWrapper::~I2CWrapper() {
    if (is_init_) {
        i2c_driver_delete(port_);
    }
}

esp_err_t I2CWrapper::write(uint8_t slave_addr, const std::vector<uint8_t>& data) {
    if (!is_init_) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error("attempting to write when i2c is not init!");
#endif
        esp_log(kErrorLogConfig, TAG, "I2C not initialized");
        return ESP_FAIL;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data.data(), data.size(), true);
    i2c_master_stop(cmd);

    std::cout << "before wait" << std::endl;
    esp_err_t err = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(1000));
    std::cout << "after wait" << std::endl;
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(err));
    }
    return err;
}

std::tuple<esp_err_t, std::optional<std::vector<uint8_t>>> I2CWrapper::read(
  uint8_t slave_addr,
  size_t len) {
    if (!is_init_) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error("attempting to read when i2c is not init!");
#endif
        esp_log(kErrorLogConfig, TAG, "I2C not initialized");
        return {ESP_FAIL, std::nullopt};
    }
    if (len < 1) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error("attempting to read with the len of 0!");
#endif
    }

    std::vector<uint8_t> res(len);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, res.data(), len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, res.data() + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(port_, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error(std::string("Read failed! ") + esp_err_to_name(err));
#endif
        esp_log(kErrorLogConfig, TAG, "I2C read failed: %s", esp_err_to_name(err));
        return {err, std::nullopt};
    }
    return {err, res};
}

esp_err_t I2CWrapper::init() {
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_pin_;
    conf.scl_io_num = scl_pin_;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = freq_hz_;

    esp_err_t err = i2c_param_config(port_, &conf);

    if (err != ESP_OK) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error(
          std::string("Failed to set params for i2c ") + esp_err_to_name(err));
#endif
        esp_log(
          kErrorLogConfig,
          TAG,
          "Failed to set params for i2c: %s",
          esp_err_to_name(err));

        return err;
    }

    err = i2c_driver_install(port_, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
#ifdef ENABLE_SAFENET
        throw std::runtime_error(
          std::string("Failed to install driver for i2c ") + esp_err_to_name(err));
#endif
        esp_log(
          kErrorLogConfig,
          TAG,
          "Failed to install driver for i2c: %s",
          esp_err_to_name(err));

        return err;
    }

    return ESP_OK;
}
