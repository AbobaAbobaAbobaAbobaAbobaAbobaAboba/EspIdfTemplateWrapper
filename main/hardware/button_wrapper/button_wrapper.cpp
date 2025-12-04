#include "button_wrapper.hpp"
ButtonWrapper::ButtonWrapper(gpio_num_t pin) {
    button_pin_ = GPIO::GpioBasicWrapper::createPin(
      pin,
      GPIO_MODE_INPUT,
      GPIO::PullMode::PULL_DOWN);

    auto res =
      button_pin_->registerIsr([this](int level) { this->handleInterrupt(level); });
    if (!res) {
        throw std::runtime_error("Failed to register ISR for button pin");
    }

    // create timers (callbacks run in esp timer task, not ISR)
    esp_timer_create_args_t args_press{
      .callback = &ButtonWrapper::pressTimerCb,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = nullptr,
      .skip_unhandled_events = false};
    if (esp_timer_create(&args_press, &debounce_press_timer_) != ESP_OK) {
        throw std::runtime_error("Failed to create debounce press timer");
    }

    esp_timer_create_args_t args_release{
      .callback = &ButtonWrapper::releaseTimerCb,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = nullptr,
      .skip_unhandled_events = false};
    if (esp_timer_create(&args_release, &debounce_release_timer_) != ESP_OK) {
        esp_timer_delete(debounce_press_timer_);
        throw std::runtime_error("Failed to create debounce release timer");
    }
    setShortPressCallback(
      []() { esp_log(kWarnLogConfig, TAG, "Dummy short callback called!"); });
}

ButtonWrapper::~ButtonWrapper() {
    if (debounce_press_timer_) {
        esp_timer_stop(debounce_press_timer_);
        esp_timer_delete(debounce_press_timer_);
        debounce_press_timer_ = nullptr;
    }
    if (debounce_release_timer_) {
        esp_timer_stop(debounce_release_timer_);
        esp_timer_delete(debounce_release_timer_);
        debounce_release_timer_ = nullptr;
    }
}

void ButtonWrapper::setShortPressCallback(const std::function<void()>& callback) {
    press_callbacks_[0] = callback;
}

void ButtonWrapper::addHoldEvent(
  uint64_t threshold_us,
  const std::function<void()>& callback) {
    if (press_callbacks_.count(threshold_us)) {
        throw std::runtime_error("Hold event threshold already registered");
    }
    press_callbacks_[threshold_us] = callback;
}

// ISR-safe: only disable irq and start a one-shot esp_timer
void ButtonWrapper::handleInterrupt(int level) {
    if (level == 1) {  // Button pressed (assuming active high)
        if (!pressed_) {
            gpio_intr_disable(button_pin_->getPin());
            if (
              esp_timer_start_once(debounce_press_timer_, kDebounceTimeUs) != ESP_OK) {
                esp_log(kErrorLogConfig, TAG, "Failed to start debounce press timer");
                gpio_intr_enable(button_pin_->getPin());
            }
        }
    } else {  // Button released
        if (pressed_) {
            gpio_intr_disable(button_pin_->getPin());
            if (
              esp_timer_start_once(debounce_release_timer_, kDebounceTimeUs)
              != ESP_OK) {
                esp_log(kErrorLogConfig, TAG, "Failed to start debounce release timer");
                gpio_intr_enable(button_pin_->getPin());
            }
        }
    }
}

void ButtonWrapper::onPress() {
    if (button_pin_->read() == 1) {  // Still pressed after debounce
        pressed_ = true;
        press_start_time_ = esp_timer_get_time();
    }
    gpio_intr_enable(button_pin_->getPin());
}

void ButtonWrapper::onRelease() {
    if (button_pin_->read() == 0) {  // Still released after debounce
        if (pressed_) {
            uint64_t duration = esp_timer_get_time() - press_start_time_;
            pressed_ = false;

            auto it = press_callbacks_.upper_bound(duration);
            --it;
            it->second();
        }
    }
    gpio_intr_enable(button_pin_->getPin());
}
