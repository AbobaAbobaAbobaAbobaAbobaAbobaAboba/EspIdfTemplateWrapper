#ifndef WEBSOCKET_HANDLER_HPP
#define WEBSOCKET_HANDLER_HPP
#include "../common/common.hpp"
#include "../common/packet.hpp"
#include "../json_wrapper/json_wrapper.hpp"
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>

class WebsocketHandler {
  public:
    WebsocketHandler();
    ~WebsocketHandler();
    WebsocketHandler(const WebsocketHandler& other) = delete;
    WebsocketHandler& operator=(const WebsocketHandler& other) = delete;
    WebsocketHandler(WebsocketHandler&& other) = delete;
    WebsocketHandler& operator=(WebsocketHandler&& other) = delete;
    esp_err_t send(std::shared_ptr<WSPacket::PacketDescriptor> descriptor);
    void close_websocket(httpd_handle_t hd, int sockfd);

  private:
    friend class HttpServerWrapper;
    static esp_err_t scheduleAsyncSend(httpd_handle_t hd);
    [[maybe_unused]] static void websocketAsyncSend(void*);

    std::mutex queue_mutex_;
    std::queue<std::shared_ptr<WSPacket::PacketDescriptor>> sending_queue_;

    inline static WebsocketHandler* instance_ = nullptr;
    inline static constexpr TagType TAG = "WEBSOCKET_HANDLER";
};
#endif