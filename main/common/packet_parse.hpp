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

std::shared_ptr<PacketBase> parse(const JsonWrapper& json) {
    auto packet_type = json.getNumber("packet_type");

#ifdef ENABLE_SAFENET
    if (packet_type == std::nullopt) {
        return {};
    }
#endif
    auto type = static_cast<PacketType>(packet_type.value());
    switch (type) {
        case PacketType::LED: {
            return parseLED(json);
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