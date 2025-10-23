#include "http_server_wrapper.hpp"
void HttpServerWrapper::init() {
    exists_ = true;
}

esp_err_t HttpServerWrapper::root_get_handler(httpd_req_t* req) {
    std::string path = "/index.html";

    esp_log(kInfoLogConfig, "HTTP_SERVER", "Requested root path: %s \n", path.c_str());

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
        auto [code, buffer, read_len] = SPIFFS::SpiffsWrapper::readFileChunked(path);

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
    esp_log(kErrorLogConfig, "HTTP_SERVER", "Received POST data: %s", buf);

    // Send response
    std::string response = "Received: " + std::string(buf);
    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

void HttpServerWrapper::resetServer() {
    server = NULL;
    config = HTTPD_DEFAULT_CONFIG();
}

bool HttpServerWrapper::initServer() {
    if (httpd_start(&server, &config) == ESP_OK) {
        esp_log(kInfoLogConfig, "HTTP_SERVER", "HTTP server started");
        return true;
    } else {
        esp_log(kErrorLogConfig, "HTTP_SERVER", "Failed to start HTTP server");
        return false;
    }
}

void HttpServerWrapper::initRoot() {
    root = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = root_get_handler,
      .user_ctx = NULL};
    httpd_register_uri_handler(server, &root);
}

void HttpServerWrapper::initPost() {
    post = {
      .uri = "/api",
      .method = HTTP_POST,
      .handler = post_handler,
      .user_ctx = NULL};
    httpd_register_uri_handler(server, &post);
}
