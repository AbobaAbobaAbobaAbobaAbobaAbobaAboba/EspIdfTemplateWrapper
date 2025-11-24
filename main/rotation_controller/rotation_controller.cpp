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

void RotationController::startRotation() {
    enable_pin_->write(1);
    // motor_pin_->write(kTicksWrite);
}

void RotationController::stopRotation() {
    enable_pin_->write(0);
    // motor_pin_->stop(MCPWM_TIMER_START_STOP_EMPTY);
}

void RotationController::setStepMode(StepMode mode) {
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
