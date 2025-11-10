#ifndef GLOBAL_HPP
#define GLOBAL_HPP
#include "../gpio_wrappers/gpio_basic_wrapper/gpio_basic_wrapper.hpp"
#include "../gpio_wrappers/mcpwm_wrapper/mcpwm_wrapper.hpp"
namespace global {

inline std::shared_ptr<GPIO::GpioBasicWrapper> led_pin;
inline std::shared_ptr<GPIO::McpwmWrapper> motor_pin;

inline std::shared_ptr<GPIO::McpwmTimer> timer1;
inline void initGlobalPins() {
    global::led_pin = GPIO::GpioBasicWrapper::createPin(
      GPIO_NUM_2,
      GPIO_MODE_OUTPUT,
      GPIO::PullMode::NONE);

    global::timer1 = GPIO::McpwmTimer::createTimer(0, 4000, 1'000'000);
    global::motor_pin = GPIO::McpwmWrapper::createPin(GPIO_NUM_25, global::timer1);
    global::motor_pin->write(1000);
}
}  // namespace global

#endif