#include "rotation_controller.hpp"
void RotationController::setDir(Direction dir) {
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

void RotationController::init() {
    timer1_ = GPIO::McpwmTimer::createTimer(0, kTicksTimer, kResolution);
    timer2_ = GPIO::McpwmTimer::createTimer(0, kTicksTimer, kResolution);
    motor_pin_1 = GPIO::McpwmWrapper::createPin(GPIO_NUM_4, timer1_);
    motor_pin_2 = GPIO::McpwmWrapper::createPin(GPIO_NUM_5, timer2_);

    dir_pin_ = GPIO::GpioBasicWrapper::createPin(
      GPIO_NUM_19,
      GPIO_MODE_OUTPUT,
      GPIO::PullMode::PULL_DOWN);
    dir_pin_->write(0);

    enable_pin_ = GPIO::GpioBasicWrapper::createPin(
      GPIO_NUM_18,
      GPIO_MODE_OUTPUT,
      GPIO::PullMode::PULL_DOWN);
    enable_pin_->write(1);
}
