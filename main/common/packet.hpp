#ifndef PACKET_HPP
#define PACKET_HPP
/* =====================================================================
 *  WSPacket - single-header packet system
 *
 *  Everything (enum, request structs, response structs, JSON parser,
 *  JSON serializer, dispatch switch) is generated from ONE table.
 *
 *  ---------------------------------------------------------------
 *  BUILDING
 *  ---------------------------------------------------------------
 *  On target (ESP-IDF):  just include this header.
 *  On host / bare test:  compile with -DWSP_STANDALONE, which swaps
 *                        esp_http_server.h + JsonWrapper for the
 *                        stubs in wsp_stubs.h. Nothing else differs.
 *
 *      g++ -std=c++17 -DWSP_STANDALONE test_packet.cpp -o test
 *      g++ -std=c++17 -DWSP_STANDALONE -DENABLE_SAFENET test_packet.cpp -o test
 *
 *  ---------------------------------------------------------------
 *  HOW TO ADD A NEW PACKET
 *  ---------------------------------------------------------------
 *  1. Declare its request fields  : WSP_REQ_FIELDS_<ENUM>(F)
 *  2. Declare its response fields : WSP_RSP_FIELDS_<ENUM>(F)
 *     (both macros MUST exist, even if empty)
 *  3. Add one line to WSP_PACKET_LIST(P): P(<ENUM>, <CamelSuffix>)
 *
 *  Field syntax:  F(<TYPE_TAG>, <name>)
 *      TYPE_TAG in { BOOL, INT, FLOAT, STRING }
 *      <name> is used both as the C++ member name and the JSON key.
 *
 *  Example:
 *      #define WSP_REQ_FIELDS_UV_SET_JOB(F) F(FLOAT, duration) F(BOOL, state)
 *      #define WSP_RSP_FIELDS_UV_SET_JOB(F)
 *      ... P(UV_SET_JOB, UvSetJob)
 *
 *  Generates:
 *      PacketType::UV_SET_JOB
 *      struct PacketUvSetJob      { double duration; bool state; }
 *      struct PacketRespUvSetJob  { RespType response; generateJson(); }
 *      Parser::parseUvSetJob(json)
 *      Parser::parse(json) dispatch entry
 *
 *  Compile with -DENABLE_SAFENET to make the parser reject packets
 *  with missing/ill-typed fields (returns a null shared_ptr).
 * ===================================================================== */

#include "../json_wrapper/json_wrapper.hpp"
#include <esp_http_server.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <tuple>

/* =====================================================================
 *  SECTION 1 - PACKET DECLARATIONS  (this is the only part you edit)
 * ===================================================================== */

/* ---- LED ---------------------------------------------------------- */
#define WSP_REQ_FIELDS_LED(F) F(BOOL, state)
#define WSP_RSP_FIELDS_LED(F)

/* ---- CREDENTIALS -------------------------------------------------- */
#define WSP_REQ_FIELDS_CREDENTIALS(F) F(STRING, ssid) F(STRING, password)
#define WSP_RSP_FIELDS_CREDENTIALS(F)


/* ---- the table ----------------------------------------------------
 *  P(<ENUM_NAME>, <CamelSuffix>)
 *  order defines the wire value of PacketType
 * ------------------------------------------------------------------- */
#define WSP_PACKET_LIST(P) \
    P(LED, Led) \
    P(CREDENTIALS, Credentials) 

/* =====================================================================
 *  SECTION 2 - TYPE TRAITS  (add a row here to support a new field type)
 * ===================================================================== */

#define WSP_CTYPE_BOOL bool
#define WSP_CTYPE_INT int
#define WSP_CTYPE_FLOAT double
#define WSP_CTYPE_STRING std::string

#define WSP_JSON_GET_BOOL(j, key) (j).getBool(key)
#define WSP_JSON_GET_INT(j, key) (j).getNumber(key)
#define WSP_JSON_GET_FLOAT(j, key) (j).getNumber(key)
#define WSP_JSON_GET_STRING(j, key) (j).getString(key)

#define WSP_JSON_ADD_BOOL(j, key, v) (j).addBool(key, v)
#define WSP_JSON_ADD_INT(j, key, v) (j).addNumber(key, v)
#define WSP_JSON_ADD_FLOAT(j, key, v) (j).addFloat(key, v)
#define WSP_JSON_ADD_STRING(j, key, v) (j).addString(key, v)

/* =====================================================================
 *  SECTION 3 - MACRO MACHINERY  (you should never need to touch this)
 * ===================================================================== */

#ifdef ENABLE_SAFENET
#define WSP_IF_SAFENET(...) __VA_ARGS__
#else
#define WSP_IF_SAFENET(...)
#endif

/* per-field emitters ------------------------------------------------ */
#define WSP_F_MEMBER(TAG, NAME) WSP_CTYPE_##TAG NAME{};
#define WSP_F_PARAM(TAG, NAME) WSP_CTYPE_##TAG _##NAME,
#define WSP_F_INIT(TAG, NAME) , NAME(_##NAME)
#define WSP_F_ADD(TAG, NAME) WSP_JSON_ADD_##TAG(json, #NAME, NAME);
#define WSP_F_FETCH(TAG, NAME) auto NAME##_opt = WSP_JSON_GET_##TAG(json, #NAME);
#define WSP_F_CHECK(TAG, NAME) \
    if (!NAME##_opt.has_value()) { \
        return {}; \
    }
#define WSP_F_ARG(TAG, NAME) ::WSPacket::detail::valueOr<WSP_CTYPE_##TAG>(NAME##_opt),

/* per-packet emitters ----------------------------------------------- */
#define WSP_G_ENUM(E, C) E,
#define WSP_G_ONE(E, C) 1 +
#define WSP_G_NAME_CASE(E, C) \
    case PacketType::E: \
        return #E;

#define WSP_G_REQ_STRUCT(E, C) \
    struct Packet##C : public PacketBase \
    { \
        static constexpr PacketType kType = PacketType::E; \
        explicit Packet##C(WSP_REQ_FIELDS_##E(WSP_F_PARAM)::WSPacket::detail::NoArg = {}) \
            : PacketBase{PacketType::E} WSP_REQ_FIELDS_##E(WSP_F_INIT) \
        {} \
        WSP_REQ_FIELDS_##E(WSP_F_MEMBER) \
    };

#define WSP_G_RSP_STRUCT(E, C) \
    struct PacketResp##C : public PacketRespBase \
    { \
        static constexpr PacketType kType = PacketType::E; \
        explicit PacketResp##C(RespType _response, \
                               WSP_RSP_FIELDS_##E(WSP_F_PARAM)::WSPacket::detail::NoArg = {}) \
            : PacketRespBase{PacketType::E, _response} WSP_RSP_FIELDS_##E(WSP_F_INIT) \
        {} \
        JsonWrapper generateJson() const override \
        { \
            JsonWrapper json = PacketRespBase::generateJson(); \
            WSP_RSP_FIELDS_##E(WSP_F_ADD) return json; \
        } \
        WSP_RSP_FIELDS_##E(WSP_F_MEMBER) \
    };

#define WSP_G_PARSER(E, C) \
    inline std::shared_ptr<PacketBase> parse##C(const JsonWrapper &json) \
    { \
        (void) json; \
        WSP_REQ_FIELDS_##E(WSP_F_FETCH) \
            WSP_IF_SAFENET(WSP_REQ_FIELDS_##E(WSP_F_CHECK)) return std::make_shared<Packet##C>( \
                WSP_REQ_FIELDS_##E(WSP_F_ARG)::WSPacket::detail::NoArg{}); \
    }

#define WSP_G_DISPATCH(E, C) \
    case PacketType::E: \
        return parse##C(json);

/* =====================================================================
 *  SECTION 4 - GENERATED CODE
 * ===================================================================== */

namespace WSPacket {

namespace detail {
/* trailing sentinel so a field list may legally be empty */
struct NoArg
{};

template<class T, class U>
inline T valueOr(const std::optional<U> &opt)
{
    return opt.has_value() ? static_cast<T>(*opt) : T{};
}
} // namespace detail

/* ---- enums -------------------------------------------------------- */
enum class PacketType : int { WSP_PACKET_LIST(WSP_G_ENUM) COUNT };

inline constexpr int kPacketCount = WSP_PACKET_LIST(WSP_G_ONE) 0;

static_assert(kPacketCount == static_cast<int>(PacketType::COUNT),
              "PacketType::COUNT mismatch - table and enum are out of sync");

enum class RespType : int {
    OK = 0,
    ERROR = 1,
    BUSY = 2,
    WARNING, // completed, but with caveats (e.g. a pending UV job was
    // force-executed to make way for the new one)
    COUNT = std::numeric_limits<int>::max()
};

enum class ParseResult : uint8_t { OK, NOT_SUPPORTED, ERROR };

inline const char *toString(PacketType t)
{
    switch (t) {
        WSP_PACKET_LIST(WSP_G_NAME_CASE)
    default:
        return "UNKNOWN";
    }
}

/* ---- transport ---------------------------------------------------- */
struct PacketDescriptor
{
    httpd_ws_frame_t frame;
    httpd_handle_t hd;
    int fd;
    std::shared_ptr<char> str;
};

/* ---- bases -------------------------------------------------------- */
struct PacketBase
{
    PacketType packet_type;

    explicit PacketBase(PacketType _type)
        : packet_type{_type}
    {}
    virtual ~PacketBase() = default;
};

struct PacketRespBase : public PacketBase
{
    RespType response;

    PacketRespBase(PacketType _type, RespType _resp)
        : PacketBase{_type}
        , response{_resp}
    {}

    virtual JsonWrapper generateJson() const
    {
        JsonWrapper json;
        json.addNumber("packet_type", static_cast<int>(packet_type));
        json.addNumber("response", static_cast<int>(response));
        return json;
    }
};

/* ---- request packets ---------------------------------------------- */
WSP_PACKET_LIST(WSP_G_REQ_STRUCT)

/* ---- response packets --------------------------------------------- */
WSP_PACKET_LIST(WSP_G_RSP_STRUCT)

/* ---- safe downcast ------------------------------------------------
 *  auto job = WSPacket::packet_cast<WSPacket::PacketUvSetJob>(p);
 *  if (job) { ... job->duration ... }
 * ------------------------------------------------------------------- */
template<class T>
inline std::shared_ptr<T> packet_cast(const std::shared_ptr<PacketBase> &p)
{
    if (!p || p->packet_type != T::kType) {
        return nullptr;
    }
    return std::static_pointer_cast<T>(p);
}

/* ---- parser ------------------------------------------------------- */
namespace Parser {

WSP_PACKET_LIST(WSP_G_PARSER)

inline std::shared_ptr<PacketBase> parse(const JsonWrapper &json)
{
    auto packet_type = json.getNumber("packet_type");

    if (!packet_type.has_value()) {
        return {};
    }

    const auto raw = static_cast<int>(packet_type.value());
    if (raw < 0 || raw >= kPacketCount) {
        return {};
    }

    switch (static_cast<PacketType>(raw)) {
        WSP_PACKET_LIST(WSP_G_DISPATCH)
    default:
        return {};
    }
}

} // namespace Parser

/* ---- serializer ---------------------------------------------------
 *  The returned frame.payload points INTO the returned shared_ptr,
 *  so keep the shared_ptr alive until httpd_ws_send_frame() returns.
 * ------------------------------------------------------------------- */
inline std::tuple<httpd_ws_frame_t, std::shared_ptr<char>> constructFrame(
    const PacketRespBase &packet)
{
    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    JsonWrapper json = packet.generateJson();
    auto serialized = json.serialize();

    frame.payload = reinterpret_cast<uint8_t *>(serialized.get());
    frame.len = serialized ? strlen(serialized.get()) : 0;
    frame.type = HTTPD_WS_TYPE_TEXT;

    return {frame, serialized};
}

} // namespace WSPacket

/* =====================================================================
 *  SECTION 5 - CLEANUP
 *  Generator macros are undef'd so they don't leak into user code.
 *  WSP_PACKET_LIST / WSP_*_FIELDS_* are kept: they are useful if you
 *  want to generate your own tables (handlers, tests, docs...).
 * ===================================================================== */

#undef WSP_F_MEMBER
#undef WSP_F_PARAM
#undef WSP_F_INIT
#undef WSP_F_ADD
#undef WSP_F_FETCH
#undef WSP_F_CHECK
#undef WSP_F_ARG
#undef WSP_G_ENUM
#undef WSP_G_ONE
#undef WSP_G_NAME_CASE
#undef WSP_G_REQ_STRUCT
#undef WSP_G_RSP_STRUCT
#undef WSP_G_PARSER
#undef WSP_G_DISPATCH

#endif