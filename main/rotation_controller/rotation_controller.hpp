#ifdef ENABLE_ROTATION_CONTROLLER
#ifndef ROTATION_CONTROLLER_HPP
#define ROTATION_CONTROLLER_HPP
#include "../common/common.hpp"
#include "../gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.hpp"
#include "../gpio_wrappers/mcpwm_wrapper/mcpwm_wrapper.hpp"
static constexpr uint32_t kTicksTimer = 32000;
static constexpr uint32_t kTicksWrite = 16000;
static constexpr uint32_t kResolution = 1'000'000;
class RotationController {
  public:
    
    RotationController() { init(); }
    RotationController(const RotationController& other) = delete;
    RotationController(RotationController&& other) = delete;
    RotationController& operator=(const RotationController& other) = delete;
    RotationController& operator=(RotationController&& other) = delete;
    void setDir(Direction dir);
    void startRotation();
    void stopRotation();

    void rotate(int steps) {
        if (steps <= 0) {
            throw std::runtime_error("Failed to rotate due to invalid steps value");
        }
        enable_pin_->write(0);
        vTaskDelay(pdMS_TO_TICKS(10));
        for (int i = 0; i < steps; ++i) {
            motor_pin_->write(1);
            esp_rom_delay_us(1);
            motor_pin_->write(0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        enable_pin_->write(1);
    }

    void setStepMode(StepMode mode);

  private:
    void init();

    // std::shared_ptr<GPIO::McpwmTimer> timer1_;
    // std::shared_ptr<GPIO::McpwmWrapper> motor_pin_;
    std::shared_ptr<GPIO::GpioBasicWrapper> motor_pin_;
    std::shared_ptr<GPIO::GpioBasicWrapper> dir_pin_;
    std::shared_ptr<GPIO::GpioBasicWrapper> enable_pin_;
    std::shared_ptr<GPIO::GpioBasicWrapper> m0_pin_;
    std::shared_ptr<GPIO::GpioBasicWrapper> m1_pin_;
    std::shared_ptr<GPIO::GpioBasicWrapper> m2_pin_;
};

#endif
#endif // ROTATION_CONTROLLER_HPP