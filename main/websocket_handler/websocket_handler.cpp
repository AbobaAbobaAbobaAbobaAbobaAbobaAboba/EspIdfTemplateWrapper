#include "websocket_handler.hpp"
#include <mutex>
WebsocketHandler::WebsocketHandler() {
#ifdef ENABLE_SAFENET
    if (instance_ != nullptr) {
        throw std::runtime_error("WebsocketHandler already exists!");
    }
#endif
    instance_ = this;
}
WebsocketHandler::~WebsocketHandler() {
#ifdef ENABLE_SAFENET
    if (instance_ == nullptr) {
        esp_log(kErrorLogConfig, WebsocketHandler::TAG, "Instance is already null!");
    }
#endif
}
esp_err_t WebsocketHandler::send(
  std::shared_ptr<WSPacket::PacketDescriptor> descriptor) {
    // const WSPacket::PacketRespBase* packet
    std::lock_guard<std::mutex> lock(queue_mutex_);
    sending_queue_.push(descriptor);
    return scheduleAsyncSend(descriptor->hd);
}
void WebsocketHandler::close_websocket(httpd_handle_t hd, int sockfd) {
    httpd_ws_frame_t close_pkt = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_CLOSE,
      .payload = nullptr,
      .len = 0};
    httpd_ws_send_frame_async(hd, sockfd, &close_pkt);

    // Then close the socket
    httpd_sess_trigger_close(hd, sockfd);
}
esp_err_t WebsocketHandler::scheduleAsyncSend(httpd_handle_t hd) {
    return httpd_queue_work(hd, websocketAsyncSend, NULL);
}

void WebsocketHandler::websocketAsyncSend(void*) {
#ifdef ENABLE_SAFENET
    if (instance_->sending_queue_.empty()) {
        throw std::runtime_error("dequeuing from an empty websocket sending queue");
    }
#endif
    std::shared_ptr<WSPacket::PacketDescriptor> descriptor;
    {
        std::lock_guard<std::mutex> lock(instance_->queue_mutex_);
        descriptor = instance_->sending_queue_.front();
        instance_->sending_queue_.pop();
    }
    auto err =
      httpd_ws_send_frame_async(descriptor->hd, descriptor->fd, &descriptor->frame);
    if (err != ESP_OK) {
#ifdef ENABLE_SAFENET
        std::string error_str = esp_err_to_name(err);
        throw std::runtime_error(
          "Failed to send websocket async response: " + error_str);
#endif
    }
}
