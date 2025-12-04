// ...existing code...
#ifndef BUTTON_WRAPPER_HPP
#define BUTTON_WRAPPER_HPP

#include "../../gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.hpp"
#include <driver/gpio.h>
#include <esp_timer.h>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

class ButtonWrapper {
  public:
    ButtonWrapper(gpio_num_t pin);

    ButtonWrapper(const ButtonWrapper& other) = delete;
    ButtonWrapper(ButtonWrapper&& other) = delete;

    ButtonWrapper& operator=(const ButtonWrapper& other) = delete;
    ButtonWrapper& operator=(ButtonWrapper&& other) = delete;

    ~ButtonWrapper();

    void setShortPressCallback(const std::function<void()>& callback);

    void addHoldEvent(uint64_t threshold_us, const std::function<void()>& callback);
    gpio_num_t getPin() { return button_pin_->getPin(); }

  private:
    void handleInterrupt(int level);

    // esp_timer callbacks must be static C-style functions
    static void pressTimerCb(void* arg) { static_cast<ButtonWrapper*>(arg)->onPress(); }
    static void releaseTimerCb(void* arg) {
        static_cast<ButtonWrapper*>(arg)->onRelease();
    }

    void onPress();
    void onRelease();

    std::shared_ptr<GPIO::GpioBasicWrapper> button_pin_;
    bool pressed_ = false;
    uint64_t press_start_time_ = 0;
    std::map<uint64_t, std::function<void()>> press_callbacks_;

    esp_timer_handle_t debounce_press_timer_ = nullptr;
    esp_timer_handle_t debounce_release_timer_ = nullptr;

    inline static constexpr TagType TAG = "BUTTON_WRAPPER";
    static constexpr uint64_t kDebounceTimeUs = 10000;
};
#endif
// ...existing code...