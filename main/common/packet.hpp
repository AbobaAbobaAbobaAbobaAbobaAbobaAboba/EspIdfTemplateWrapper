#ifndef PACKET_HPP
#define PACKET_HPP
#include "../json_wrapper/json_wrapper.hpp"
#include "../rotation_controller/rotation_controller.hpp"
#include <esp_http_server.h>
#include <limits>
#include <memory>
#include <optional>
#include <string>
namespace WSPacket {

enum class PacketType : int {
    LED = 0,
    CREDENTIALS,
    ZOOM,
    FOCUS,
    CHANGE_DIRECTION,
    COUNT
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
    std::shared_ptr<char> str;
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
struct PacketCredentials : public PacketBase {
    PacketCredentials(std::string _ssid, std::string _password)
        : PacketBase{PacketType::CREDENTIALS},
          ssid(_ssid),
          password(_password) {}
    std::string ssid;
    std::string password;
};

struct PacketZoom : public PacketBase {
    PacketZoom(bool _state) : PacketBase{PacketType::ZOOM}, state{_state} {}
    bool state;
};

struct PacketFocus : public PacketBase {
    PacketFocus(bool _state) : PacketBase{PacketType::FOCUS}, state{_state} {}
    bool state;
};

struct PacketChangeDirection : public PacketBase {
    PacketChangeDirection(bool _dir)
        : PacketBase{PacketType::CHANGE_DIRECTION},
          state{_dir} {}
    bool state;
};

static_assert(
  5 == static_cast<int>(PacketType::COUNT),
  "PacketType::COUNT mismatch. Check if enough Packet structs exist");

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

struct PacketRespCredentials : public PacketRespBase {
    PacketRespCredentials(RespType _resp)
        : PacketRespBase{PacketType::CREDENTIALS, _resp} {}

    virtual JsonWrapper generateJson() const override {
        return PacketRespBase::generateJson();
    }
};

struct PacketRespZoom : public PacketRespBase {
    PacketRespZoom(RespType _resp) : PacketRespBase{PacketType::ZOOM, _resp} {}

    virtual JsonWrapper generateJson() const override {
        return PacketRespBase::generateJson();
    }
};

struct PacketRespFocus : public PacketRespBase {
    PacketRespFocus(RespType _resp) : PacketRespBase{PacketType::FOCUS, _resp} {}

    virtual JsonWrapper generateJson() const override {
        return PacketRespBase::generateJson();
    }
};

struct PacketRespChangeDirection : public PacketRespBase {
    PacketRespChangeDirection(RespType _resp)
        : PacketRespBase{PacketType::CHANGE_DIRECTION, _resp} {}

    virtual JsonWrapper generateJson() const override {
        return PacketRespBase::generateJson();
    }
};

static_assert(
  5 == static_cast<int>(PacketType::COUNT),
  "PacketType::COUNT mismatch. Check if enough RESP structs exist");

};  // namespace WSPacket

#endif