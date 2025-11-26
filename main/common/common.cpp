#include "common.hpp"
#include "../littlefs_wrapper/littlefs_wrapper.hpp"
#include "../scheduler/scheduler.hpp"

void initializeSystem() {
    // System initialization

    esp_err_t err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    err = esp_netif_init();
    ESP_ERROR_CHECK(err);

    err = esp_event_loop_create_default();
    ESP_ERROR_CHECK(err);

    if (err != ESP_OK) {
        SystemStatus::system_status_ = false;
    } else {
        SystemStatus::system_status_ = true;
    }
    LittleFS::LittleFsWrapper::init();

    Scheduler::Scheduler::init();
}