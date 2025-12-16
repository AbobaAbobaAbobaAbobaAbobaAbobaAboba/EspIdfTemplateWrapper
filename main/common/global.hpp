#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "../gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.hpp"

namespace global {

inline std::shared_ptr<GPIO::GpioBasicWrapper> led_pin;
inline std::shared_ptr<GPIO::GpioBasicWrapper> external_led_pin;

constexpr std::uint64_t millisToMicros(std::uint64_t milliseconds) {
    return milliseconds * 1000ULL;
}
inline void initGlobalPins() {
    global::led_pin = GPIO::GpioBasicWrapper::createPin(
      GPIO_NUM_8,
      GPIO_MODE_OUTPUT,
      GPIO::PullMode::NONE);
    global::external_led_pin = GPIO::GpioBasicWrapper::createPin(
      GPIO_NUM_10,
      GPIO_MODE_OUTPUT,
      GPIO::PullMode::PULL_UP);
}

inline void blinkLED(int interval = 100) {
    global::led_pin->write(0);
    vTaskDelay(pdMS_TO_TICKS(interval));
    global::led_pin->write(1);
    vTaskDelay(pdMS_TO_TICKS(interval));
}

inline void blinkExternalLED(int interval = 100) {
    global::external_led_pin->write(1);
    vTaskDelay(pdMS_TO_TICKS(interval));
    global::external_led_pin->write(0);
    vTaskDelay(pdMS_TO_TICKS(interval));
}
inline void resetAndHoldPins() {
    global::external_led_pin->write(0);
    global::external_led_pin->setHold(true);
    global::led_pin->write(1);
    global::led_pin->setHold(true);
    gpio_deep_sleep_hold_en();
}
inline void disableHold() {
    global::external_led_pin->setHold(false);
    global::led_pin->setHold(false);
    gpio_deep_sleep_hold_dis();
}

}  // namespace global

#endif