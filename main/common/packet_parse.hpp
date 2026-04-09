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

std::shared_ptr<PacketBase> parseCredentials(const JsonWrapper& json) {
    auto ssid = json.getString("ssid");
    auto password = json.getString("password");

#ifdef ENABLE_SAFENET
    if (!ssid.has_value() || !password.has_value()) {
        return {};
    }
#endif

    std::shared_ptr<PacketBase> packet =
      std::make_shared<PacketCredentials>(ssid.value(), password.value());
    return packet;
}

std::shared_ptr<PacketBase> parseUvIsActive(const JsonWrapper& json) {
    std::shared_ptr<PacketBase> packet = std::make_shared<PacketUvIsActive>();
    return packet;
}

std::shared_ptr<PacketBase> parseUvSetJob(const JsonWrapper& json) {
    auto duration_opt = json.getBool("duration");
    auto state_opt = json.getBool("state");

#ifdef ENABLE_SAFENET
    if (duration_opt == std::nullopt) {
        return {};
    }
    if (state_opt == std::nullopt) {
        return {};
    }
#endif

    std::shared_ptr<PacketBase> packet =
      std::make_shared<PacketUvSetJob>(duration_opt.value(), state_opt.value());
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
        case PacketType::CREDENTIALS: {
            return parseCredentials(json);
        }
        case PacketType::UV_IS_ACTIVE: {
            return parseUvIsActive(json);
        }
        case PacketType::UV_SET_JOB: {
            return parseUvSetJob(json);
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