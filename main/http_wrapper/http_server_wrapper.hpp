#ifndef HTTP_SERVER_WRAPPER_HPP
#define HTTP_SERVER_WRAPPER_HPP
#include "../common/common.hpp"
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
#include <vector>
class HttpServerWrapper {
  public:
    HttpServerWrapper() {
#ifdef ENABLE_SAFENET
        if (exists_) {
            throw std::runtime_error("WifiHandler already exists!");
        }
#endif
        init();
    }
    ~HttpServerWrapper() {
    }

    HttpServerWrapper(const HttpServerWrapper& other) = delete;
    HttpServerWrapper& operator=(const HttpServerWrapper& other) = delete;
    HttpServerWrapper(HttpServerWrapper&& other) = delete;
    HttpServerWrapper& operator=(HttpServerWrapper&& other) = delete;
    bool start_webserver(void) {
        resetServer();

        if (initServer() == false) {
            return false;
        }
        esp_log(kInfoLogConfig, "HTTP_SERVER", "HTTP server started");
        initRoot();
        initPost();
        initUriHandlers();
        for (int i = 0; i < uri_handlers_.size(); ++i) {
            esp_log(
              kInfoLogConfig,
              "HTTP_SERVER",
              "Registered URI handler for %s \n",
              uri_handlers_[i].uri);
        }
        return true;
    }

  private:
    static esp_err_t root_get_handler(httpd_req_t* req);
    static esp_err_t post_handler(httpd_req_t* req);
    static esp_err_t generic_get_handler(httpd_req_t* req) {
        std::string path = req->uri;

        esp_log(kInfoLogConfig, "HTTP_SERVER", "Requested path: %s \n", path.c_str());

        // Set content type
        if (strstr(path.c_str(), ".html")) {
            httpd_resp_set_type(req, "text/html");
        } else if (strstr(path.c_str(), ".css")) {
            httpd_resp_set_type(req, "text/css");
        } else if (strstr(path.c_str(), ".js")) {
            httpd_resp_set_type(req, "application/javascript");
        }

        bool continue_read = true;
        while (continue_read) {
            auto [code, buffer, read_len] =
              SPIFFS::SpiffsWrapper::readFileChunked(path);

            switch (code) {
                case SPIFFS::ErrorCode::CONTINUE_READ:
                    httpd_resp_send_chunk(req, buffer, read_len);
                    break;
                case SPIFFS::ErrorCode::FINISHED_READ:
                    httpd_resp_send_chunk(req, buffer, read_len);
                    continue_read = false;
                    break;
                case SPIFFS::ErrorCode::FILE_NOT_FOUND:
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
#if ENABLE_SAFENET
    inline static std::atomic<bool> exists_ = false;
#endif

    void init();
    void initUriHandlers() {
        // debug this shit later
        // I (13446) HTTP_SERVER: Registered URI handler for css/app.2cf79ad6.css8��?
        //  sI (13456) HTTP_SERVER: Registered URI handler for js/app.559277fc.8��?R
        resetPaths();
        uri_paths_ = SPIFFS::SpiffsWrapper::getPaths();
        for (int i = 0, is = uri_paths_.size(); i < is; ++i) {
            uri_paths_[i] = "/" + uri_paths_[i];
        }
        for (int i = 0; i < uri_paths_.size(); ++i) {
            esp_log(
              kInfoLogConfig,
              "HTTP_SERVER",
              "Read paths %s",
              uri_paths_[i].c_str());
        }
        uri_handlers_.clear();
        uri_handlers_.resize(uri_paths_.size());
        for (int i = 0; i < uri_paths_.size(); ++i) {
            httpd_uri_t uri_handler;
            memset(&uri_handler, 0, sizeof(httpd_uri_t));
            uri_handler = {
              .uri = uri_paths_[i].c_str(),
              .method = HTTP_GET,
              .handler = generic_get_handler,
              .user_ctx = NULL};
            uri_handlers_[i] = uri_handler;
            httpd_register_uri_handler(server, &uri_handlers_[i]);
        }
    }
    bool initServer();
    void initRoot();
    void initPost();
    void resetPaths() {
        uri_paths_.clear();
    }
    void resetServer();
    std::vector<std::string> uri_paths_;
    httpd_handle_t server;
    httpd_config_t config;
    httpd_uri_t root;
    httpd_uri_t post;

    std::vector<httpd_uri_t> uri_handlers_;
};
#endif  // HTTP_SERVER_WRAPPER_HPP