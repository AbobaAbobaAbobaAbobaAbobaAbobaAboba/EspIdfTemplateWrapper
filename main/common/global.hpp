#ifndef GLOBAL_HPP
#define GLOBAL_HPP
#include "../gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.hpp"
#include "../gpio_wrappers/mcpwm_wrapper/mcpwm_wrapper.hpp"
namespace global {

inline std::shared_ptr<GPIO::GpioBasicWrapper> led_pin;

inline std::shared_ptr<GPIO::McpwmWrapper> pwm_pin;

inline void initGlobalPins() {
    global::led_pin = GPIO::GpioBasicWrapper::createPin(
      GPIO_NUM_2,
      GPIO_MODE_OUTPUT,
      GPIO::PullMode::NONE);

    global::pwm_pin = GPIO::McpwmWrapper::createPin(GPIO_NUM_18, 3030);

    // write 1 to init the single shot mechanism
    // more of a workaround but beats
    // handling singleshot on my own
    // with sync mechanism
}
}  // namespace global

#endif