#ifndef PACKET_HPP
#define PACKET_HPP
#include "../json_wrapper/json_wrapper.hpp"
#include <esp_http_server.h>
#include <limits>
#include <memory>
#include <optional>
namespace WSPacket {

enum class PacketType : int {
    LED = 0
};
enum class RespType : int {
    OK = 0,
    ERROR = 1,
    BUSY = 2,
    COUNT = std::numeric_limits<int>::max()
};
struct PacketDescriptor {
    httpd_ws_frame_t frame;
    httpd_handle_t hd;
    int fd;
    std::shared_ptr<JsonWrapper> json;
};

enum class ParseResult : uint8_t {
    OK,
    NOT_SUPPORTED,
    ERROR
};

struct PacketBase {
    PacketType packet_type;
    PacketBase(PacketType _type) : packet_type{_type} {}

    virtual ~PacketBase() = default;
};

struct PacketLED : public PacketBase {
    PacketLED(bool _state) : PacketBase{PacketType::LED}, state{_state} {}
    bool state;
};
// ========
struct PacketRespBase : public PacketBase {
    PacketRespBase(PacketType _type, RespType _resp)
        : PacketBase{_type},
          response{_resp} {}

    virtual JsonWrapper generateJson() const {
        JsonWrapper json;
        json.addNumber("packet_type", static_cast<int>(packet_type));
        json.addNumber("response", static_cast<int>(response));
        return json;
    }

    RespType response;
};

struct PacketRespLED : public PacketRespBase {
    PacketRespLED(RespType _resp) : PacketRespBase{PacketType::LED, _resp} {}

    virtual JsonWrapper generateJson() const override {
        return PacketRespBase::generateJson();
    }
};

};  // namespace WSPacket

#endif