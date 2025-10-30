#include "websocket_handler.hpp"

esp_err_t WebsocketHandler::scheduleAsyncSend(httpd_handle_t hd) {
    esp_log(kErrorLogConfig, "WEBSOCKET_HANDLER", "aboba queueing \n");
    return httpd_queue_work(hd, websocketAsyncSend, NULL);
}

void WebsocketHandler::websocketAsyncSend(void*) {
    esp_log(kErrorLogConfig, "WEBSOCKET_HANDLER", "aboba 1 \n");
#ifdef ENABLE_SAFENET
    if (instance_->sending_queue_.empty()) {
        throw std::runtime_error("dequeuing from an empty websocket sending queue");
    }
#endif
    esp_log(kErrorLogConfig, "WEBSOCKET_HANDLER", "aboba 2 \n");
    std::shared_ptr<WSPacket::PacketDescriptor> descriptor;
    {
        std::lock_guard<std::mutex> lock(instance_->queue_mutex_);
        descriptor = instance_->sending_queue_.front();
        instance_->sending_queue_.pop();
    }
    esp_log(kErrorLogConfig, "WEBSOCKET_HANDLER", "aboba 3 \n");
    auto err =
      httpd_ws_send_frame_async(descriptor->hd, descriptor->fd, &descriptor->frame);
    esp_log(kErrorLogConfig, "WEBSOCKET_HANDLER", "aboba 4 \n");
    if (err != ESP_OK) {
#ifdef ENABLE_SAFENET
        std::string error_str = esp_err_to_name(err);
        throw std::runtime_error(
          "Failed to send websocket async response: " + error_str);
#endif
    }
}
