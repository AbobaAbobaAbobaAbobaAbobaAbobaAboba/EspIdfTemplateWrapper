#include "common.hpp"
#include "../littlefs_wrapper/littlefs_wrapper.hpp"
#include "../scheduler/scheduler.hpp"
#include <driver/gpio.h>
void initializeSystem() {
    // System initialization

    esp_err_t err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    err = esp_netif_init();
    ESP_ERROR_CHECK(err);

    err = esp_event_loop_create_default();
    ESP_ERROR_CHECK(err);

    err =
      customErrorCheck(gpio_install_isr_service(0), "Failed to init ISR GPIO service");

    if (err != ESP_OK) {
        SystemStatus::system_status_ = false;
    } else {
        SystemStatus::system_status_ = true;
    }
    LittleFS::LittleFsWrapper::init();

    Scheduler::Scheduler::init();
}