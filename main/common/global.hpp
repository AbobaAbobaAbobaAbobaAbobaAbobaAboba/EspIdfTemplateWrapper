#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include "../gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.hpp"
#include "../gpio_wrappers/mcpwm_wrapper/mcpwm_wrapper.hpp"
namespace global {

inline std::shared_ptr<GPIO::GpioBasicWrapper> led_pin;

inline void initGlobalPins() {
    global::led_pin = GPIO::GpioBasicWrapper::createPin(
      GPIO_NUM_2,
      GPIO_MODE_OUTPUT,
      GPIO::PullMode::NONE);
}

inline void blinkLED(int interval = 100) {
    global::led_pin->write(1);
    vTaskDelay(pdMS_TO_TICKS(interval));
    global::led_pin->write(0);
    vTaskDelay(pdMS_TO_TICKS(interval));
}
}  // namespace global

#endif