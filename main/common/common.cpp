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
    initializeWifiStack();
    // led = GPIO::GpioBasicWrapper::getInstance(LED_PIN);
    // if (led->isConfigured() == false) {
    //     led->configure(GPIO_MODE_OUTPUT, GPIO::PullMode::NONE);
    // }
    Scheduler::Scheduler::init();
}
void initializeWifiStack() {
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    esp_err_t err = esp_wifi_init(&cfg);
    ESP_ERROR_CHECK(err);

    if (err != ESP_OK) {
        SystemStatus::wifi_status_ = false;
    } else {
        SystemStatus::wifi_status_ = true;
    }
}