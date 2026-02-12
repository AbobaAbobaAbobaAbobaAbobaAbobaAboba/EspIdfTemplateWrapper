
#ifndef ROTATION_CONTROLLER_HPP
#define ROTATION_CONTROLLER_HPP
#include "../common/common.hpp"
#include "../gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.hpp"
#include "../gpio_wrappers/mcpwm_wrapper/mcpwm_wrapper.hpp"
static constexpr uint32_t kTicksTimer = 250;
static constexpr uint32_t kTicksWrite = 50;
static constexpr uint32_t kResolution = 1'000'000;
class RotationController {
  public:
    enum class Direction : uint8_t {
        CLOCKWISE = 0,
        COUNTER_CLOCKWISE
    };
    enum class StepMode : uint8_t {
        STEP_FULL = 0,
        STEP_1_2,
        STEP_1_4,
        STEP_1_8,
        STEP_1_16,
        STEP_1_32,
    };
    enum class Motor : uint8_t {
        MOTOR_1 = 0,
        MOTOR_2 = 1
    };
    RotationController() { init(); }
    RotationController(const RotationController& other) = delete;
    RotationController(RotationController&& other) = delete;
    RotationController& operator=(const RotationController& other) = delete;
    RotationController& operator=(RotationController&& other) = delete;
    void setDir(Direction dir);
    void startRotation(Motor motor) {
        enable_pin_->write(0);
        if (motor == Motor::MOTOR_1) {
            motor_pin_1->write(kTicksWrite);
        } else if (motor == Motor::MOTOR_2) {
            motor_pin_2->write(kTicksWrite);
        } else {
            throw std::runtime_error("Failed to handle motor enum");
        }
    }
    void stopRotation(Motor motor) {
        if (motor == Motor::MOTOR_1) {
            motor_pin_1->stop();
        } else if (motor == Motor::MOTOR_2) {
            motor_pin_2->stop();
        } else {
            throw std::runtime_error("Failed to handle motor enum");
        }
        enable_pin_->write(1);
    }

    // void rotate(int steps) {
    //     if (steps <= 0) {
    //         throw std::runtime_error("Failed to rotate due to invalid steps value");
    //     }
    //     enable_pin_->write(0);
    //     vTaskDelay(pdMS_TO_TICKS(10));
    //     for (int i = 0; i < steps; ++i) {
    //         motor_pin_->write(1);
    //         esp_rom_delay_us(1);
    //         motor_pin_->write(0);
    //         vTaskDelay(pdMS_TO_TICKS(10));
    //     }
    //     enable_pin_->write(1);
    // }

  private:
    void init();

    std::shared_ptr<GPIO::McpwmTimer> timer1_;
    std::shared_ptr<GPIO::McpwmTimer> timer2_;
    std::shared_ptr<GPIO::McpwmWrapper> motor_pin_1;
    std::shared_ptr<GPIO::McpwmWrapper> motor_pin_2;
    std::shared_ptr<GPIO::GpioBasicWrapper> dir_pin_;
    std::shared_ptr<GPIO::GpioBasicWrapper> enable_pin_;
};

#endif
