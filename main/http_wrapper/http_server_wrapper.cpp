#include "http_server_wrapper.hpp"
#include "../common/packet_construct.hpp"
#include "../common/packet_parse.hpp"
#include "../wifi_wrapper/wifi_base.hpp"
void HttpServerWrapper::init() {
    exists_ = true;
}

void HttpServerWrapper::initUriHandlers() {
    resetPaths();

    std::vector<std::string> temp_paths = LittleFS::LittleFsWrapper::getPaths();

    uri_paths_.clear();

    for (auto& path : temp_paths) {
        if (
          path.find(web_folder) == std::string::npos
          || path.find("index.html") != std::string::npos) {
            continue;
        }
        uri_paths_.push_back(path);
    }

    uri_handlers_.resize(uri_paths_.size());

    for (int i = 0; i < uri_paths_.size(); ++i) {
        httpd_uri_t uri_handler;
        memset(&uri_handler, 0, sizeof(httpd_uri_t));

        uri_handler = {
          .uri = uri_paths_[i].c_str() + sizeof(web_folder) - 1,
          .method = HTTP_GET,
          .handler = generic_get_handler,
          .user_ctx = NULL,
          .is_websocket = false,
          .handle_ws_control_frames = false,
          .supported_subprotocol = NULL};
        uri_handlers_[i] = uri_handler;
        httpd_register_uri_handler(server_, &uri_handlers_[i]);
    }
}

HttpServerWrapper::HttpServerWrapper() {
#ifdef ENABLE_SAFENET
    if (exists_) {
        throw std::runtime_error("StationWrapper already exists!");
    }
#endif
    instance_ = this;
    init();
}

HttpServerWrapper::~HttpServerWrapper() {
    exists_ = false;
    instance_ = nullptr;
}

bool HttpServerWrapper::startWebserver(void) {
    resetServer();

    if (initServer() == false) {
        return false;
    }

    esp_log(kInfoLogConfig, HttpServerWrapper::TAG, "HTTP server started");

    initRoot();

    initPost();

    initUriHandlers();

    initConfig();

    initWebSocket();

    for (int i = 0; i < uri_handlers_.size(); ++i) {
        esp_log(
          kInfoLogConfig,
          HttpServerWrapper::TAG,
          "Registered URI handler for %s \n",
          uri_handlers_[i].uri);
    }
    return true;
}

esp_err_t HttpServerWrapper::sendPacket(
  const WSPacket::PacketRespBase* packet,
  int sockfd) {
    switch (packet->packet_type) {
        case WSPacket::PacketType::LED: {
            auto [frame, json] = WSPacket::constructFrame(
              *static_cast<const WSPacket::PacketRespLED*>(packet));

            std::shared_ptr<WSPacket::PacketDescriptor> descriptor =
              std::make_shared<WSPacket::PacketDescriptor>(
                frame,
                server_,
                sockfd,
                json);
            ws_handler_.send(descriptor);
            return ESP_OK;
        }
        default: {
            return ESP_FAIL;
        }
    }
    return ESP_ERR_NOT_SUPPORTED;
}

void HttpServerWrapper::invokeCallback(
  std::shared_ptr<WSPacket::PacketBase> packet,
  int sockfd) {
#ifdef ENABLE_SAFENET
    if (packet == nullptr) {
        throw std::runtime_error("Packet pointer is null");
    }
#endif
    callback_(packet, sockfd);
}

esp_err_t HttpServerWrapper::root_get_handler(httpd_req_t* req) {
    esp_err_t err = instance_->handleSocketCount();
    if (err != ESP_OK) {
        static constexpr char resp[] =
          "HTTP/1.1 503 Service Unavailable\r\n"
          "Content-Type: text/plain\r\n"
          "Connection: close\r\n"
          "Retry-After: 5\r\n"  // optional: tell client to retry after 5s
          "\r\n"
          "The amount of clients is maxed out. Try to connect later.\r\n";

        esp_log(
          kErrorLogConfig,
          HttpServerWrapper::TAG,
          "Clients are maxed out. Dropping the connection...");
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send_chunk(req, resp, sizeof(resp));
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }
    std::string path = generatePath("/index.html");

    esp_log(
      kInfoLogConfig,
      HttpServerWrapper::TAG,
      "Requested root path: %s \n",
      path.c_str());

    // Set content type
    if (strstr(path.c_str(), ".html")) {
        httpd_resp_set_type(req, "text/html");
    } else if (strstr(path.c_str(), ".css")) {
        httpd_resp_set_type(req, "text/css");
    } else if (strstr(path.c_str(), ".js")) {
        httpd_resp_set_type(req, "application/javascript");
    }

    bool continue_read = true;
    auto read_descriptor = LittleFS::LittleFsWrapper::open(path);
    while (continue_read) {
        auto [code, buffer, read_len] = read_descriptor.readFileChunked();

        switch (code) {
            case LittleFS::ErrorCode::CONTINUE_READ:
                httpd_resp_send_chunk(req, buffer, read_len);
                break;
            case LittleFS::ErrorCode::FINISHED_READ:
                httpd_resp_send_chunk(req, buffer, read_len);
                continue_read = false;
                break;
            case LittleFS::ErrorCode::FILE_NOT_FOUND:
                httpd_resp_send_404(req);
                continue_read = false;
                break;
            default:
                httpd_resp_send_500(req);
                continue_read = false;
                break;
        }
    }
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t HttpServerWrapper::post_handler(httpd_req_t* req) {
    char buf[100];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request body too large");
        return ESP_FAIL;
    }

    // Read the POST data
    ret = httpd_req_recv(req, buf, std::min(remaining, (int)sizeof(buf)));
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Null-terminate the received data
    buf[ret] = '\0';
    esp_log(kErrorLogConfig, HttpServerWrapper::TAG, "Received POST data: %s", buf);

    // Send response
    std::string response = "Received: " + std::string(buf);
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

esp_err_t HttpServerWrapper::generic_get_handler(httpd_req_t* req) {
    std::string path = generatePath(req->uri);

    esp_log(
      kInfoLogConfig,
      HttpServerWrapper::TAG,
      "Requested path: %s \n",
      path.c_str());

    // Set content type
    if (strstr(path.c_str(), ".html")) {
        httpd_resp_set_type(req, "text/html");
    } else if (strstr(path.c_str(), ".css")) {
        httpd_resp_set_type(req, "text/css");
    } else if (strstr(path.c_str(), ".js")) {
        httpd_resp_set_type(req, "application/javascript");
    }

    bool continue_read = true;
    auto read_descriptor = LittleFS::LittleFsWrapper::open(path);
    while (continue_read) {
        auto [code, buffer, read_len] = read_descriptor.readFileChunked();

        switch (code) {
            case LittleFS::ErrorCode::CONTINUE_READ:
                httpd_resp_send_chunk(req, buffer, read_len);
                break;
            case LittleFS::ErrorCode::FINISHED_READ:
                httpd_resp_send_chunk(req, buffer, read_len);
                continue_read = false;
                break;
            case LittleFS::ErrorCode::FILE_NOT_FOUND:
                httpd_resp_send_404(req);
                continue_read = false;
                break;
            default:
                httpd_resp_send_500(req);
                continue_read = false;
                break;
        }
    }
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t HttpServerWrapper::config_handler(httpd_req_t* req) {
    JsonWrapper json;
    json.addString("ip", Wifi::Base::getCurrentIp());
    httpd_resp_set_type(req, "application/json");
    auto str = json.serialize();
    httpd_resp_send(req, str.get(), strlen(str.get()));
    esp_log(
      kInfoLogConfig,
      HttpServerWrapper::TAG,
      "Served config with IP %s",
      Wifi::Base::getCurrentIp().c_str());
    return ESP_OK;
}

esp_err_t HttpServerWrapper::websocket_handler(httpd_req_t* req) {
    esp_log(
      kInfoLogConfig,
      HttpServerWrapper::TAG,
      "Handler invoked with %d",
      req->method);
    if (req->method == HTTP_GET) {
        esp_log(
          kInfoLogConfig,
          HttpServerWrapper::TAG,
          "Handshake done, the new connection was opened");
        esp_err_t err = instance_->handleSocketCount();
        return err;
    }

    httpd_ws_frame_t ws_pkt;

    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);

    if (ret != ESP_OK) {
        esp_log(
          kErrorLogConfig,
          HttpServerWrapper::TAG,
          "httpd_ws_recv_frame failed to get frame len with %d",
          ret);

        instance_->ws_handler_.close_websocket(
          instance_->server_,
          httpd_req_to_sockfd(req));

        return ret;
    }

    if (ws_pkt.len == 0 && ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        instance_->ws_handler_.close_websocket(
          instance_->server_,
          httpd_req_to_sockfd(req));

        esp_log(kInfoLogConfig, HttpServerWrapper::TAG, "Client inited disconnect");
        return ESP_OK;
    }

    std::unique_ptr<uint8_t[]> buf(new uint8_t[ws_pkt.len + 1]);
    buf[ws_pkt.len] = '\0';
    ws_pkt.payload = buf.get();

    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);

    if (ret != ESP_OK) {
        esp_log(
          kErrorLogConfig,
          HttpServerWrapper::TAG,
          "httpd_ws_recv_frame failed with %d",
          ret);
        instance_->ws_handler_.close_websocket(
          instance_->server_,
          httpd_req_to_sockfd(req));
        return ret;
    }

    esp_log(
      kInfoLogConfig,
      HttpServerWrapper::TAG,
      "Got packet with message: %s",
      ws_pkt.payload);

    esp_log(kInfoLogConfig, HttpServerWrapper::TAG, "frame len is %d", ws_pkt.len);

    if (ws_pkt.type != HTTPD_WS_TYPE_TEXT) {
        esp_log(
          kErrorLogConfig,
          HttpServerWrapper::TAG,
          "ws_pkt.type != HTTPD_WS_TYPE_TEXT");
        instance_->ws_handler_.close_websocket(
          instance_->server_,
          httpd_req_to_sockfd(req));
        return ESP_ERR_NOT_SUPPORTED;
    }

#ifdef ENABLE_SAFENET
    if (instance_ == nullptr) {
        throw std::runtime_error("HttpServerWrapper instance is null");
    }
#endif

    std::shared_ptr<JsonWrapper> json = std::make_shared<JsonWrapper>(ws_pkt.payload);

    std::shared_ptr<WSPacket::PacketBase> packet = WSPacket::Parser::parse(*json);
    if (!packet) {
        esp_log(kErrorLogConfig, HttpServerWrapper::TAG, "Failed to parse!");
        instance_->ws_handler_.close_websocket(
          instance_->server_,
          httpd_req_to_sockfd(req));
        return ESP_ERR_INVALID_ARG;
    }

    instance_->invokeCallback(packet, httpd_req_to_sockfd(req));

    size_t free_dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t total_dram = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t used_dram = total_dram - free_dram;
    esp_log(kErrorLogConfig, HttpServerWrapper::TAG, "===RAM %d !", used_dram);
    return ESP_OK;
}

void HttpServerWrapper::resetServer() {
    server_ = NULL;
    config_ = HTTPD_DEFAULT_CONFIG();
    config_.max_uri_handlers = 16;
    config_.max_open_sockets = kMaxActiveSocketsHard;
    config_.lru_purge_enable = true;  // <--- THIS LINE
}

int HttpServerWrapper::getSocketCount() {
    int client_fds[CONFIG_LWIP_MAX_SOCKETS];
    size_t num_active =
      CONFIG_LWIP_MAX_SOCKETS;  // In: Size of array; Out: Number of active FDs
    esp_err_t ret = httpd_get_client_list(server_, &num_active, client_fds);
    if (ret != ESP_OK) {
        return -1;
    }
    return num_active;
}

esp_err_t HttpServerWrapper::handleSocketCount() {
    int socket_count = getSocketCount();
    esp_log(kErrorLogConfig, TAG, "current socket count: %d", socket_count);

    if (socket_count == -1) {
        return ESP_FAIL;
    }

    if (socket_count >= kMaxActiveSocketsSoft) {
        esp_log(kErrorLogConfig, HttpServerWrapper::TAG, "Clients are maxed out.");
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool HttpServerWrapper::initServer() {
    if (httpd_start(&server_, &config_) == ESP_OK) {
        esp_log(kInfoLogConfig, HttpServerWrapper::TAG, "HTTP server started");
        return true;
    } else {
        esp_log(kErrorLogConfig, HttpServerWrapper::TAG, "Failed to start HTTP server");
        return false;
    }
}

void HttpServerWrapper::initRoot() {
    root_ = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = root_get_handler,
      .user_ctx = NULL,
      .is_websocket = false,
      .handle_ws_control_frames = false,
      .supported_subprotocol = NULL};
    httpd_register_uri_handler(server_, &root_);
}

void HttpServerWrapper::initPost() {
    post_ = {
      .uri = "/api",
      .method = HTTP_POST,
      .handler = post_handler,
      .user_ctx = NULL,
      .is_websocket = false,
      .handle_ws_control_frames = false,
      .supported_subprotocol = NULL};
    httpd_register_uri_handler(server_, &post_);
}

void HttpServerWrapper::initConfig() {
    config_uri_ = {
      .uri = "/config",
      .method = HTTP_GET,
      .handler = config_handler,
      .user_ctx = NULL,
      .is_websocket = false,
      .handle_ws_control_frames = false,
      .supported_subprotocol = NULL};
    httpd_register_uri_handler(server_, &config_uri_);
}

void HttpServerWrapper::initWebSocket() {
    ws_uri_ = {
      .uri = "/ws",
      .method = HTTP_GET,
      .handler = websocket_handler,
      .user_ctx = NULL,
      .is_websocket = true,
      .handle_ws_control_frames = false,
      .supported_subprotocol = NULL};
    httpd_register_uri_handler(server_, &ws_uri_);
}

void HttpServerWrapper::resetPaths() {
    uri_paths_.clear();
}
