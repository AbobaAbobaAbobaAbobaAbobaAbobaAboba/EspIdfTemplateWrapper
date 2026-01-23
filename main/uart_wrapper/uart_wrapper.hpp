#ifndef UART_WRAPPER_HPP
#define UART_WRAPPER_HPP

#include "../common/common.hpp"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "soc/uart_reg.h"
// uart_config_t uart_config = {
//           .baud_rate = 115200,                    // Common baud rate
//           .data_bits = UART_DATA_8_BITS,          // 8 data bits
//           .parity = UART_PARITY_DISABLE,          // No parity
//           .stop_bits = UART_STOP_BITS_1,          // 1 stop bit
//           .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,  // No hardware flow control
//           .rx_flow_ctrl_thresh = 0,               // Not used with flow control
//           disabled .source_clk = UART_SCLK_DEFAULT,        // Default clock source
//           .flags = {0, 0}                         // No signal inversions or other
//           flags
//         };
class UartWrapper {
  public:
    UartWrapper(
      uart_port_t port,
      int tx_io_num,
      int rx_io_num,
      const uart_config_t& config)
        : port_(port) {
        if (instance_) {
            throw std::runtime_error("An instance of UartWrapper already exists!");
        }
        instance_ = this;
        configure(port, tx_io_num, rx_io_num, config);
    }
    UartWrapper(const UartWrapper& other) = delete;
    UartWrapper(UartWrapper&& other) = delete;
    UartWrapper& operator=(const UartWrapper& other) = delete;
    UartWrapper& operator=(UartWrapper&& other) = delete;
    ~UartWrapper() { instance_ = nullptr; }

  private:
    uart_port_t port_;
    inline static constexpr TagType TAG = "UART_WRAPPER";
    inline static constexpr size_t kBufferSize = 512;
    inline static std::array<uint8_t, kBufferSize * 2> buffer;
    static constexpr int kQueueSize = 10;
    inline static UartWrapper* instance_ = nullptr;
    inline static QueueHandle_t uart_queue = {};
    static void worker(void* pvParameters) {
        for (;;) {
            uart_event_t event;
            if (xQueueReceive(uart_queue, (void*)&event, (TickType_t)portMAX_DELAY)) {
                switch (event.type) {
                    case UART_DATA:
                        ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                        uart_read_bytes(
                          instance_->port_,
                          buffer.data(),
                          event.size,
                          portMAX_DELAY);
                        ESP_LOGI(TAG, "[DATA EVT]:");
                        uart_write_bytes(
                          instance_->port_,
                          (const char*)buffer.data(),
                          event.size);
                        esp_log(
                          kInfoLogConfig,
                          TAG,
                          "Data size sending %d",
                          event.size);
                        break;
                    // Event of HW FIFO overflow detected
                    case UART_FIFO_OVF:
                        ESP_LOGI(TAG, "hw fifo overflow");
                        // If fifo overflow happened, you should consider adding flow
                        // control for your application. The ISR has already reset the
                        // rx FIFO, As an example, we directly flush the rx buffer here
                        // in order to read more data.
                        uart_flush_input(instance_->port_);
                        xQueueReset(uart_queue);
                        break;
                    // Event of UART ring buffer full
                    case UART_BUFFER_FULL:
                        ESP_LOGI(TAG, "ring buffer full");
                        // If buffer full happened, you should consider increasing your
                        // buffer size As an example, we directly flush the rx buffer
                        // here in order to read more data.
                        uart_flush_input(instance_->port_);
                        xQueueReset(uart_queue);
                        break;
                    // Event of UART RX break detected
                    case UART_BREAK:
                        ESP_LOGI(TAG, "uart rx break");
                        break;
                    // Event of UART parity check error
                    case UART_PARITY_ERR:
                        ESP_LOGI(TAG, "uart parity error");
                        break;
                    // Event of UART frame error
                    case UART_FRAME_ERR:
                        ESP_LOGI(TAG, "uart frame error");
                        break;
                    default:
                        ESP_LOGI(TAG, "uart event type: %d", event.type);
                        break;
                }
            }
        }
    }
    void configure(
      uart_port_t port,
      int tx_io_num,
      int rx_io_num,
      const uart_config_t& config) {
        customErrorCheck(
          uart_driver_install(
            port,
            kBufferSize,
            kBufferSize,
            kQueueSize,
            &uart_queue,
            0),
          "Failed to install uart driver");
        customErrorCheck(
          uart_param_config(port, &config),
          "failed to set param config uart");
        customErrorCheck(
          uart_set_pin(
            port,
            tx_io_num,
            rx_io_num,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE),
          "failed to set uart pin");

        xTaskCreate(&UartWrapper::worker, "uart_event_task", 3072, NULL, 12, NULL);
    }
};
#endif