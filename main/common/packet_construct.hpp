#ifndef PACKET_CONSTRUCT_HPP
#define PACKET_CONSTRUCT_HPP
#include "packet.hpp"
#include <esp_http_server.h>
#include <memory>
#include <stdexcept>
namespace WSPacket {

inline std::tuple<httpd_ws_frame_t, std::shared_ptr<JsonWrapper>> constructFrame(
  const WSPacket::PacketRespLED& packet) {
    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    std::shared_ptr<JsonWrapper> json =
      std::make_shared<JsonWrapper>(packet.generateJson());

    char* serialized = json->serialize();

    frame.payload = reinterpret_cast<uint8_t*>(serialized);
    frame.len = strlen(serialized);
    frame.type = HTTPD_WS_TYPE_TEXT;

    return {frame, json};
}

inline std::tuple<httpd_ws_frame_t, std::shared_ptr<JsonWrapper>> constructFrame(
  const WSPacket::PacketRespServo& packet) {
    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    std::shared_ptr<JsonWrapper> json =
      std::make_shared<JsonWrapper>(packet.generateJson());

    char* serialized = json->serialize();

    frame.payload = reinterpret_cast<uint8_t*>(serialized);
    frame.len = strlen(serialized);
    frame.type = HTTPD_WS_TYPE_TEXT;

    return {frame, json};
}

};  // namespace WSPacket
#endif