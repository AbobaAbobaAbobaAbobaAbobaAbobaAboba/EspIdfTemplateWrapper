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

std::shared_ptr<PacketBase> parseZoom(const JsonWrapper& json) {
    auto state_opt = json.getBool("state");

#ifdef ENABLE_SAFENET
    if (state_opt == std::nullopt) {
        return {};
    }
#endif

    std::shared_ptr<PacketBase> packet =
      std::make_shared<PacketZoom>(state_opt.value());
    return packet;
}

std::shared_ptr<PacketBase> parseFocus(const JsonWrapper& json) {
    auto state_opt = json.getBool("state");

#ifdef ENABLE_SAFENET
    if (state_opt == std::nullopt) {
        return {};
    }
#endif

    std::shared_ptr<PacketBase> packet =
      std::make_shared<PacketFocus>(state_opt.value());
    return packet;
}

std::shared_ptr<PacketBase> parseChangeDirection(const JsonWrapper& json) {
    auto state_opt = json.getBool("state");

#ifdef ENABLE_SAFENET
    if (state_opt == std::nullopt) {
        return {};
    }
#endif

    std::shared_ptr<PacketBase> packet =
      std::make_shared<PacketChangeDirection>(state_opt.value());
    return packet;
}
static_assert(
  5 == static_cast<int>(PacketType::COUNT),
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
      5 == static_cast<int>(PacketType::COUNT),
      "PacketType::COUNT mismatch. Check if enough Packet structs exist");
    switch (type) {
        case PacketType::LED: {
            return parseLED(json);
        }
        case PacketType::CREDENTIALS: {
            return parseCredentials(json);
        }
        case PacketType::ZOOM: {
            return parseZoom(json);
        }
        case PacketType::FOCUS: {
            return parseFocus(json);
        }
        case PacketType::CHANGE_DIRECTION: {
            return parseChangeDirection(json);
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