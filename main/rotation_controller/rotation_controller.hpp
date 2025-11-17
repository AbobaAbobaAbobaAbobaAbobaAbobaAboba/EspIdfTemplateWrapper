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
    RotationController() { init(); }
    RotationController(const RotationController& other) = delete;
    RotationController(RotationController&& other) = delete;
    RotationController& operator=(const RotationController& other) = delete;
    RotationController& operator=(RotationController&& other) = delete;
    void setDir(Direction dir) {
        switch (dir) {
            case Direction::CLOCKWISE:
                dir_pin_->write(1);
                break;
            case Direction::COUNTER_CLOCKWISE:
                dir_pin_->write(0);
                break;
            default: {
                throw std::runtime_error("Unknown direction!");
            }
        }
    };
    void startRotation() {
        enable_pin_->write(1);
        // motor_pin_->write(kTicksWrite);
    }
    void stopRotation() {
        enable_pin_->write(0);
        // motor_pin_->stop(MCPWM_TIMER_START_STOP_EMPTY);
    }

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
            vTaskDelay(pdMS_TO_TICKS(30));
        }
        enable_pin_->write(1);
    }

    void setStepMode(StepMode mode) {
        switch (mode) {
            case StepMode::STEP_FULL:
                m0_pin_->write(0);
                m1_pin_->write(0);
                m2_pin_->write(0);
                break;
            case StepMode::STEP_1_2:
                m0_pin_->write(1);
                m1_pin_->write(0);
                m2_pin_->write(0);
                break;
            case StepMode::STEP_1_4:
                m0_pin_->write(0);
                m1_pin_->write(1);
                m2_pin_->write(0);
                break;
            case StepMode::STEP_1_8:
                m0_pin_->write(1);
                m1_pin_->write(1);
                m2_pin_->write(0);
                break;
            case StepMode::STEP_1_16:
                m0_pin_->write(0);
                m1_pin_->write(0);
                m2_pin_->write(1);
                break;
            case StepMode::STEP_1_32:
                m0_pin_->write(1);
                m1_pin_->write(1);
                m2_pin_->write(1);
                break;
            default: {
                throw std::runtime_error("Unknown step mode!");
            }
        }
    }

  private:
    void init() {
        // timer1_ = GPIO::McpwmTimer::createTimer(0, kTicksTimer, kResolution);
        // motor_pin_ = GPIO::McpwmWrapper::createPin(GPIO_NUM_25, timer1_);

        motor_pin_ = GPIO::GpioBasicWrapper::createPin(
          GPIO_NUM_25,
          GPIO_MODE_OUTPUT,
          GPIO::PullMode::PULL_DOWN);
        motor_pin_->write(0);

        dir_pin_ = GPIO::GpioBasicWrapper::createPin(
          GPIO_NUM_32,
          GPIO_MODE_OUTPUT,
          GPIO::PullMode::PULL_DOWN);
        dir_pin_->write(0);

        enable_pin_ = GPIO::GpioBasicWrapper::createPin(
          GPIO_NUM_33,
          GPIO_MODE_OUTPUT,
          GPIO::PullMode::PULL_UP);
        enable_pin_->write(1);

        m0_pin_ = GPIO::GpioBasicWrapper::createPin(
          GPIO_NUM_5,
          GPIO_MODE_OUTPUT,
          GPIO::PullMode::PULL_DOWN);
        m0_pin_->write(0);

        m1_pin_ = GPIO::GpioBasicWrapper::createPin(
          GPIO_NUM_17,
          GPIO_MODE_OUTPUT,
          GPIO::PullMode::PULL_DOWN);
        m1_pin_->write(0);

        m2_pin_ = GPIO::GpioBasicWrapper::createPin(
          GPIO_NUM_18,
          GPIO_MODE_OUTPUT,
          GPIO::PullMode::PULL_DOWN);
        m2_pin_->write(0);
        setStepMode(StepMode::STEP_FULL);
    }

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