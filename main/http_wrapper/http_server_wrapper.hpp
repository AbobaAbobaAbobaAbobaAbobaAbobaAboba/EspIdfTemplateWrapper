#ifndef HTTP_SERVER_WRAPPER_HPP
#define HTTP_SERVER_WRAPPER_HPP
#include "../common/common.hpp"
#include "../json_wrapper/json_wrapper.hpp"
#include "../spiffs_wrapper/spiffs_wrapper.hpp"
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <atomic>
#include <exception>
#include <string>
#include <unordered_set>
#include <vector>
enum class AsyncRespCode : int {
    LED = 0
};

namespace WSPacket {

#pragma pack(push, 1)

enum class ParseResult : uint8_t {
    OK,
    NOT_SUPPORTED,
    ERROR
};

struct AsyncRespBase {
    AsyncRespCode packet_type;
};

struct AsyncRespLed : public AsyncRespBase {
    bool state;
};

struct PacketDescriptor {
    httpd_req_t* req;
    JsonWrapper* json_ptr;
};

#pragma pack(pop)

};  // namespace WSPacket

class HttpServerWrapper {
  public:
    HttpServerWrapper();
    ~HttpServerWrapper();

    HttpServerWrapper(const HttpServerWrapper& other) = delete;
    HttpServerWrapper& operator=(const HttpServerWrapper& other) = delete;
    HttpServerWrapper(HttpServerWrapper&& other) = delete;
    HttpServerWrapper& operator=(HttpServerWrapper&& other) = delete;
    bool startWebserver(void);

  private:
    static esp_err_t root_get_handler(httpd_req_t* req);
    static esp_err_t post_handler(httpd_req_t* req);
    static esp_err_t generic_get_handler(httpd_req_t* req);
    static esp_err_t config_handler(httpd_req_t* req);
    static esp_err_t websocket_handler(httpd_req_t* req);

    static esp_err_t scheduleAsyncSend(httpd_req_t* req, JsonWrapper* json);
    static void websocketAsyncSend(void* arg);
    static httpd_ws_frame_t handleWebsocketSendLed(WSPacket::AsyncRespLed* packet);

#if ENABLE_SAFENET
    inline static std::atomic<bool> exists_ = false;
#endif

    void init();
    void initUriHandlers();
    bool initServer();
    void initRoot();
    void initPost();
    void initConfig();
    void initWebSocket();

    void resetPaths();
    void resetServer();
    esp_err_t handleWsLED(JsonWrapper* json);

    std::vector<std::string> uri_paths_;
    httpd_handle_t server_;
    httpd_config_t config_;
    httpd_uri_t root_;
    httpd_uri_t post_;
    httpd_uri_t config_uri_;
    httpd_uri_t ws_uri_;
    std::vector<httpd_uri_t> uri_handlers_;

    inline static HttpServerWrapper* instance_ = nullptr;
};
#endif  // HTTP_SERVER_WRAPPER_HPP