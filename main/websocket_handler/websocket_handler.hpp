#ifndef WEBSOCKET_HANDLER_HPP
#define WEBSOCKET_HANDLER_HPP
#include "../common/common.hpp"
#include "../common/packet.hpp"
#include "../json_wrapper/json_wrapper.hpp"
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <queue>
#include <stdexcept>

class WebsocketHandler {
  public:
    WebsocketHandler() {
#ifdef ENABLE_SAFENET
        if (instance_ != nullptr) {
            throw std::runtime_error("WebsocketHandler already exists!");
        }
#endif
        instance_ = this;
    }
    ~WebsocketHandler() {
#ifdef ENABLE_SAFENET
        if (instance_ == nullptr) {
            esp_log(kErrorLogConfig, "WEBSOCKET_HANDLER", "Instance is already null!");
        }
#endif
    }
    WebsocketHandler(const WebsocketHandler& other) = delete;
    WebsocketHandler& operator=(const WebsocketHandler& other) = delete;
    WebsocketHandler(WebsocketHandler&& other) = delete;
    WebsocketHandler& operator=(WebsocketHandler&& other) = delete;
    esp_err_t send(std::shared_ptr<WSPacket::PacketDescriptor> descriptor) {
        // const WSPacket::PacketRespBase* packet
        std::lock_guard<std::mutex> lock(queue_mutex_);
        sending_queue_.push(descriptor);
        return scheduleAsyncSend(descriptor->hd);
    }

  private:
    friend class HttpServerWrapper;
    static esp_err_t scheduleAsyncSend(httpd_handle_t hd);
    [[maybe_unused]] static void websocketAsyncSend(void*);

    std::mutex queue_mutex_;
    std::queue<std::shared_ptr<WSPacket::PacketDescriptor>> sending_queue_;

    inline static WebsocketHandler* instance_ = nullptr;
};
#endif