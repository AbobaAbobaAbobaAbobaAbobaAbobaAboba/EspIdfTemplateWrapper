#ifndef HTTP_SERVER_WRAPPER_HPP
#define HTTP_SERVER_WRAPPER_HPP
#include "../common/common.hpp"
#include "../json_wrapper/json_wrapper.hpp"
#include "../littlefs_wrapper/littlefs_wrapper.hpp"
#include "../websocket_handler/websocket_handler.hpp"
#include "lwip/sockets.h"
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <atomic>
#include <exception>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>
class HttpServerWrapper {
    using Callback = std::function<void(std::shared_ptr<WSPacket::PacketBase>, int)>;

  public:
    static_assert(
      CONFIG_LWIP_MAX_SOCKETS - 4 > 0,
      "CONFIG_LWIP_MAX_SOCKETS must exceed 4");

    static_assert(
      CONFIG_LWIP_MAX_ACTIVE_TCP - 4 >= 0,
      "CONFIG_LWIP_MAX_ACTIVE_TCP must exceed 4");

    static_assert(
      CONFIG_LWIP_MAX_SOCKETS - CONFIG_LWIP_MAX_ACTIVE_TCP >= 4,
      "CONFIG_LWIP_MAX_SOCKETS has to have at least 4 TCP sockets");

    /// @brief the max amount of supported sockets. This one is limited by the hardware
    constexpr static uint8_t kMaxActiveSocketsHard = CONFIG_LWIP_MAX_ACTIVE_TCP - 3;

    /// @brief leaving 1 socket for telling the user to try to connect later
    constexpr static uint8_t kMaxActiveSocketsSoft = kMaxActiveSocketsHard - 1;

    HttpServerWrapper();
    ~HttpServerWrapper();

    HttpServerWrapper(const HttpServerWrapper& other) = delete;
    HttpServerWrapper& operator=(const HttpServerWrapper& other) = delete;
    HttpServerWrapper(HttpServerWrapper&& other) = delete;
    HttpServerWrapper& operator=(HttpServerWrapper&& other) = delete;
    bool startWebserver(void);
    void registerCallback(Callback callback) { callback_ = callback; }
    void invokeCallback(std::shared_ptr<WSPacket::PacketBase> packet, int sockfd);
    esp_err_t sendPacket(const WSPacket::PacketRespBase* packet, int sockfd);

  private:
    static esp_err_t onSocketOpened(httpd_handle_t hd, int sockfd);

    static esp_err_t root_get_handler(httpd_req_t* req);
    static esp_err_t post_handler(httpd_req_t* req);
    static esp_err_t generic_get_handler(httpd_req_t* req);
    static esp_err_t config_handler(httpd_req_t* req);
    static esp_err_t websocket_handler(httpd_req_t* req);

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
    int getSocketCount();
    esp_err_t handleSocketCount();
    httpd_handle_t server_;
    httpd_config_t config_;
    httpd_uri_t root_;
    httpd_uri_t post_;
    httpd_uri_t config_uri_;
    httpd_uri_t ws_uri_;

    Callback callback_;
    WebsocketHandler ws_handler_;
    std::vector<std::string> uri_paths_;
    std::vector<httpd_uri_t> uri_handlers_;
    inline static HttpServerWrapper* instance_ = nullptr;
    inline static constexpr TagType TAG = "HTTP_SERVER";

    static std::string generatePath(const std::string& filepath) {
        return std::string(web_folder) + filepath;
    }
    constexpr static char web_folder[] = "/web_do_not_rename";
};
#endif  // HTTP_SERVER_WRAPPER_HPP