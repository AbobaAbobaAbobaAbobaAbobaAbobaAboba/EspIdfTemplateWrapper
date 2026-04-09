#ifndef PACKET_CONSTRUCT_HPP
#define PACKET_CONSTRUCT_HPP
#include "packet.hpp"
#include <esp_http_server.h>
#include <memory>
#include <stdexcept>
namespace WSPacket {

inline std::tuple<httpd_ws_frame_t, std::shared_ptr<char>> constructFrame(
  const WSPacket::PacketRespBase& packet) {
    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    std::shared_ptr<JsonWrapper> json =
      std::make_shared<JsonWrapper>(packet.generateJson());

    auto serialized = json->serialize();

    frame.payload = reinterpret_cast<uint8_t*>(serialized.get());
    frame.len = strlen(serialized.get());
    frame.type = HTTPD_WS_TYPE_TEXT;

    return {frame, serialized};
}

};  // namespace WSPacket
#endif