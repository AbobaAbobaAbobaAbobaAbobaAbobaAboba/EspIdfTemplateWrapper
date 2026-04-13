#ifndef PACKET_HPP
#define PACKET_HPP
#include "../json_wrapper/json_wrapper.hpp"
#include <esp_http_server.h>
#include <limits>
#include <memory>
#include <optional>
#include <string>
namespace WSPacket {

enum class PacketType : int {
    LED = 0,
    CREDENTIALS,
    UV_GET_STATUS,
    UV_SET_JOB,
    COUNT
};
enum class RespType : int {
    OK = 0,
    ERROR = 1,
    BUSY = 2,
    WARNING,  // task was completed successful but with some issues (e.g. a pending UV
              // job was force executed to make way for the new one)
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
// UV_GET_STATUS request {packet_type: 2}
// UV_SET_JOB  request
//{
// packet_type: 3,
// (float)duration: 15.2(seconds);
// (bool) state: true/false(true - perform job, false - stop the job);
// }

// enum class RespType : int {
//     OK = 0,
//     ERROR = 1,
//     BUSY = 2,
//     WARNING,   task was completed successful but with some issues (e.g. a pending
//     UV job was force executed to make way for the new one)
//     COUNT = std::numeric_limits<int>::max()
// };
// UV_GET_STATUS response {
// packet_type: 2
// (RespType)response: RespType
// (uint64_t) remaining_time: time
// (bool) state: true/false
// }

// UV_SET_JOB  response
// {
// packet_type: 3,
// (RespType)response: RespType
// }

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
// UV_GET_STATUS,
// UV_SET_JOB,
struct PacketUvIsActive : public PacketBase {
    PacketUvIsActive() : PacketBase{PacketType::UV_GET_STATUS} {}
};

struct PacketUvSetJob : public PacketBase {
    PacketUvSetJob(double _duration, bool _state)
        : PacketBase{PacketType::UV_SET_JOB},
          duration(_duration),
          state(_state) {}
    double duration;
    bool state;
};

static_assert(
  4 == static_cast<int>(PacketType::COUNT),
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

struct PacketRespUvIsActive : public PacketRespBase {
    PacketRespUvIsActive(double _remaining_time, bool _state, RespType _resp)
        : PacketRespBase{PacketType::UV_GET_STATUS, _resp},
          remaining_time(_remaining_time),
          state(_state) {}

    virtual JsonWrapper generateJson() const override {
        auto base = PacketRespBase::generateJson();
        base.addFloat("remaining_time", remaining_time);
        base.addBool("state", state);
        return base;
    }
    double remaining_time;
    bool state;
};

struct PacketRespUvSetJob : public PacketRespBase {
    PacketRespUvSetJob(RespType _resp)
        : PacketRespBase{PacketType::UV_SET_JOB, _resp} {}

    virtual JsonWrapper generateJson() const override {
        return PacketRespBase::generateJson();
    }
};

static_assert(
  4 == static_cast<int>(PacketType::COUNT),
  "PacketType::COUNT mismatch. Check if enough RESP structs exist");
};  // namespace WSPacket

#endif