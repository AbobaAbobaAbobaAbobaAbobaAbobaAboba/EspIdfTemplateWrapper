// ...existing code...
#ifndef BUTTON_WRAPPER_HPP
#define BUTTON_WRAPPER_HPP

#include "../../gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.hpp"
#include <driver/gpio.h>
#include <esp_timer.h>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

class ButtonWrapper {
  public:
    ButtonWrapper(gpio_num_t pin) {
        button_pin_ = GPIO::GpioBasicWrapper::createPin(
          pin,
          GPIO_MODE_INPUT,
          GPIO::PullMode::PULL_DOWN);

        auto res =
          button_pin_->registerIsr([this](int level) { this->handleInterrupt(level); });
        if (!res) {
            throw std::runtime_error("Failed to register ISR for button pin");
        }
        button_name_ = "Button_GPIO_" + std::to_string(static_cast<int>(pin));

        // create timers (callbacks run in esp timer task, not ISR)
        esp_timer_create_args_t args_press{
          .callback = &ButtonWrapper::debouncePressTimerCb,
          .arg = this,
          .dispatch_method = ESP_TIMER_TASK,
          .name = nullptr,
          .skip_unhandled_events = false};
        if (esp_timer_create(&args_press, &debounce_press_timer_) != ESP_OK) {
            throw std::runtime_error("Failed to create debounce press timer");
        }

        esp_timer_create_args_t args_release{
          .callback = &ButtonWrapper::debounceReleaseTimerCb,
          .arg = this,
          .dispatch_method = ESP_TIMER_TASK,
          .name = nullptr,
          .skip_unhandled_events = false};
        if (esp_timer_create(&args_release, &debounce_release_timer_) != ESP_OK) {
            esp_timer_delete(debounce_press_timer_);
            throw std::runtime_error("Failed to create debounce release timer");
        }

        esp_timer_create_args_t args_long{
          .callback = &ButtonWrapper::longPressTimerCb,
          .arg = this,
          .dispatch_method = ESP_TIMER_TASK,
          .name = nullptr,
          .skip_unhandled_events = false};
        if (esp_timer_create(&args_long, &long_press_timer_) != ESP_OK) {
            esp_timer_delete(debounce_press_timer_);
            esp_timer_delete(debounce_release_timer_);
            throw std::runtime_error("Failed to create long press timer");
        }
    }

    ButtonWrapper(const ButtonWrapper& other) = delete;
    ButtonWrapper(ButtonWrapper&& other) = delete;

    ButtonWrapper& operator=(const ButtonWrapper& other) = delete;
    ButtonWrapper& operator=(ButtonWrapper&& other) = delete;

    ~ButtonWrapper() {
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
        if (long_press_timer_) {
            esp_timer_stop(long_press_timer_);
            esp_timer_delete(long_press_timer_);
            long_press_timer_ = nullptr;
        }
        // Note: Unregistering ISR should be handled by the GPIO wrapper if needed.
    }

    void setShortPressCallback(const std::function<void()>& callback) {
        short_press_callback_ = callback;
    }

    void setLongPressCallback(const std::function<void()>& callback) {
        long_press_callback_ = callback;
    }

  private:
    std::shared_ptr<GPIO::GpioBasicWrapper> button_pin_;
    std::string button_name_;
    std::function<void()> short_press_callback_;
    std::function<void()> long_press_callback_;
    bool pressed_ = false;
    uint64_t press_start_time_ = 0;

    esp_timer_handle_t debounce_press_timer_ = nullptr;
    esp_timer_handle_t debounce_release_timer_ = nullptr;
    esp_timer_handle_t long_press_timer_ = nullptr;

    inline static constexpr TagType TAG = "BUTTON_WRAPPER";
    static constexpr uint64_t kDebounceTimeUs = 50000;         // 50ms
    static constexpr uint64_t kLongPressThresholdUs = 300000;  // 300ms

    // ISR-safe: only disable irq and start a one-shot esp_timer
    void handleInterrupt(int level) {
        if (level == 1) {  // Button pressed (assuming active high)
            if (!pressed_) {
                gpio_intr_disable(button_pin_->getPin());
                if (
                  esp_timer_start_once(debounce_press_timer_, kDebounceTimeUs)
                  != ESP_OK) {
                    esp_log(
                      kErrorLogConfig,
                      TAG,
                      "Failed to start debounce press timer");
                    gpio_intr_enable(button_pin_->getPin());
                }
            }
        } else {  // Button released
            if (pressed_) {
                gpio_intr_disable(button_pin_->getPin());
                if (
                  esp_timer_start_once(debounce_release_timer_, kDebounceTimeUs)
                  != ESP_OK) {
                    esp_log(
                      kErrorLogConfig,
                      TAG,
                      "Failed to start debounce release timer");
                    gpio_intr_enable(button_pin_->getPin());
                }
            }
        }
    }

    // esp_timer callbacks must be static C-style functions
    static void debouncePressTimerCb(void* arg) {
        static_cast<ButtonWrapper*>(arg)->onDebouncePress();
    }
    static void debounceReleaseTimerCb(void* arg) {
        static_cast<ButtonWrapper*>(arg)->onDebounceRelease();
    }
    static void longPressTimerCb(void* arg) {
        static_cast<ButtonWrapper*>(arg)->onLongPress();
    }

    void onDebouncePress() {
        // runs in timer task context
        if (button_pin_->read() == 1) {  // Still pressed after debounce
            pressed_ = true;
            press_start_time_ = esp_timer_get_time();
            if (
              esp_timer_start_once(long_press_timer_, kLongPressThresholdUs)
              != ESP_OK) {
                esp_log(kErrorLogConfig, TAG, "Failed to start long press timer");
            }
        }
        gpio_intr_enable(button_pin_->getPin());
    }

    void onDebounceRelease() {
        // runs in timer task context
        if (button_pin_->read() == 0) {  // Still released after debounce
            if (pressed_) {
                uint64_t duration = esp_timer_get_time() - press_start_time_;
                pressed_ = false;
                // cancel long press timer if it hasn't fired yet
                esp_timer_stop(long_press_timer_);
                if (duration >= kDebounceTimeUs && duration < kLongPressThresholdUs) {
                    if (short_press_callback_) {
                        short_press_callback_();
                    }
                }
                // If duration >= kLongPressThresholdUs, long press handler ran already.
            }
        }
        gpio_intr_enable(button_pin_->getPin());
    }

    void onLongPress() {
        // runs in timer task context
        if (pressed_ && button_pin_->read() == 1) {  // Still pressed
            if (long_press_callback_) {
                long_press_callback_();
            }
            // keep pressed_ state; release will clear it
        }
    }
};
#endif
// ...existing code...