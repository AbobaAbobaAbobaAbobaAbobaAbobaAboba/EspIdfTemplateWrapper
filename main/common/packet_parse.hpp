#ifndef PACKET_PARSE_HPP
#define PACKET_PARSE_HPP
#include "packet.hpp"

namespace WSPacket {
namespace Parser {
std::shared_ptr<PacketBase> parseLED(const JsonWrapper& json) {
    auto state_opt = json.getBool("state");

#ifdef ENABLE_SAFENET
    if (state_opt == std::nullopt) {
        return {};
    }
#endif

    std::shared_ptr<PacketBase> packet = std::make_shared<PacketLED>(state_opt.value());
    return packet;
}

std::shared_ptr<PacketBase> parseServo(const JsonWrapper& json) {
    auto state_opt = json.getNumber("value");

#ifdef ENABLE_SAFENET
    if (state_opt == std::nullopt) {
        return {};
    }
#endif

    std::shared_ptr<PacketBase> packet =
      std::make_shared<PacketServo>(state_opt.value());
    return packet;
}

std::shared_ptr<PacketBase> parseTaskStart(const JsonWrapper& json) {
    esp_log(kInfoLogConfig, "ABOBA", "Task start");
    auto hour = json.getNumber("hour");
    auto minute = json.getNumber("minute");
    auto second = json.getNumber("second");

#ifdef ENABLE_SAFENET
    if (hour == std::nullopt || minute == std::nullopt || second == std::nullopt) {
        return {};
    }
#endif

    std::shared_ptr<PacketBase> packet =
      std::make_shared<PacketTaskStart>(hour.value(), minute.value(), second.value());
    return packet;
}

std::shared_ptr<PacketBase> parseTaskStop(const JsonWrapper& json) {
    auto id = json.getNumber("id");

#ifdef ENABLE_SAFENET
    if (id == std::nullopt) {
        return {};
    }
#endif
    std::shared_ptr<PacketBase> packet = std::make_shared<PacketTaskStop>(id.value());
    return packet;
}

static_assert(
  4 == static_cast<int>(PacketType::COUNT),
  "PacketType::COUNT mismatch. Check if enough Packet structs exist");
///====PARSE BASE====
std::shared_ptr<PacketBase> parse(const JsonWrapper& json) {
    auto packet_type = json.getNumber("packet_type");

#ifdef ENABLE_SAFENET
    if (packet_type == std::nullopt) {
        return {};
    }
#endif
    auto type = static_cast<PacketType>(packet_type.value());

    static_assert(
      4 == static_cast<int>(PacketType::COUNT),
      "PacketType::COUNT mismatch. Check if enough Packet structs exist");
    switch (type) {
        case PacketType::LED: {
            return parseLED(json);
        }
        case PacketType::SERVO: {
            return parseServo(json);
        }
        case PacketType::TASK_START: {
            return parseTaskStart(json);
        }
        case PacketType::TASK_STOP: {
            return parseTaskStop(json);
        }
        default: {
            return {};
        }
    }
    return {};
}
}  // namespace Parser
};  // namespace WSPacket
#endif