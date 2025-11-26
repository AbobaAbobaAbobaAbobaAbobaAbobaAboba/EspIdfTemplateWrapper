#include "udp_wrapper.hpp"

UdpWrapper::UdpWrapper() {
}

UdpWrapper::~UdpWrapper() {
    unlock();
}
[[nodiscard]] esp_err_t UdpWrapper::lock(
  uint16_t local_port,
  const std::string& local_ip) {
    local_port_ = local_port;
    local_ip_ = local_ip;

    sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_ < 0) {
        esp_log(kErrorLogConfig, TAG, "Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port_);

    if (local_ip_.empty()) {
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, local_ip_.c_str(), &local_addr.sin_addr) != 1) {
            esp_log(kErrorLogConfig, TAG, "Invalid IP address: %s", local_ip_.c_str());
            close(sock_);
            sock_ = -1;
            return ESP_ERR_INVALID_ARG;
        }
    }

    if (bind(sock_, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        esp_log(kErrorLogConfig, TAG, "Socket unable to bind: errno %d", errno);
        close(sock_);
        sock_ = -1;
        return ESP_FAIL;
    }

    esp_log(kInfoLogConfig, TAG, "Socket bound to port %d", local_port_);
    is_locked_ = true;
    return ESP_OK;
}

esp_err_t UdpWrapper::unlock() {
    if (sock_ >= 0) {
        close(sock_);
        sock_ = -1;
    }
    is_locked_ = false;
    return ESP_OK;
}

esp_err_t
UdpWrapper::send(const std::string& address, uint16_t port, std::string data) {
    return send(
      address,
      port,
      reinterpret_cast<const uint8_t*>(data.data()),
      data.size());
}

esp_err_t UdpWrapper::send(
  const std::string& address,  // e.g. "192.168.1.100" or "8.8.8.8"
  uint16_t port,
  const uint8_t* data,
  size_t len) {
    if (!is_locked_) {
        return ESP_ERR_INVALID_STATE;
    }

    // --------------------------------------------------------------
    // 1. Build the remote sockaddr_in structure
    // --------------------------------------------------------------
    struct sockaddr_in remote {};
    remote.sin_family = AF_INET;
    remote.sin_port = htons(port);  // network byte order

    // ---- Convert std::string → binary IPv4 address ----
    int pton_result = inet_pton(AF_INET, address.c_str(), &remote.sin_addr);

    if (pton_result != 1) {  // 1 = success, 0 = bad format, -1 = bad family
        if (pton_result == 0) {
            esp_log(
              kErrorLogConfig,
              TAG,
              "Invalid IP address format: \"%s\"",
              address.c_str());
        } else {
            esp_log(
              kErrorLogConfig,
              TAG,
              "inet_pton error for \"%s\" (errno %d)",
              address.c_str(),
              errno);
        }
        return ESP_ERR_INVALID_ARG;
    }

    // --------------------------------------------------------------
    // 2. Send the datagram
    // --------------------------------------------------------------
    int sent = sendto(
      sock_,
      data,
      len,
      0,  // flags
      (struct sockaddr*)&remote,
      sizeof(remote));

    if (sent < 0) {
        esp_log(
          kErrorLogConfig,
          TAG,
          "sendto() failed to %s:%u  errno=%d (%s)",
          address.c_str(),
          port,
          errno,
          strerror(errno));
        return ESP_FAIL;
    }

    if ((size_t)sent != len) {
        esp_log(
          kInfoLogConfig,
          TAG,
          "Partial send: requested %zu bytes, sent %d bytes",
          len,
          sent);
    }

    esp_log(kInfoLogConfig, TAG, "Sent %d bytes → %s:%u", sent, address.c_str(), port);

    return ESP_OK;
}

esp_err_t UdpWrapper::send(
  const std::string& address,
  uint16_t port,
  const std::vector<uint8_t>& data) {
    return send(address, port, data.data(), data.size());
}

std::tuple<esp_err_t, std::optional<std::vector<uint8_t>>>
UdpWrapper::recv(sockaddr_in* from, size_t max_len, int timeout_ms) {
    if (!is_locked_) {
        return {ESP_ERR_INVALID_STATE, std::nullopt};
    }

    if (timeout_ms >= 0) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock_, &readfds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(sock_ + 1, &readfds, nullptr, nullptr, &tv);
        if (ret < 0) {
            esp_log(kErrorLogConfig, TAG, "select error: errno %d", errno);
            return {ESP_FAIL, std::nullopt};
        } else if (ret == 0) {
            return {ESP_ERR_TIMEOUT, std::nullopt};
        }
    }

    std::vector<uint8_t> buffer(max_len);
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    int len = recvfrom(
      sock_,
      buffer.data(),
      max_len,
      0,
      (struct sockaddr*)&sender,
      &sender_len);
    if (len < 0) {
        esp_log(kErrorLogConfig, TAG, "recvfrom failed: errno %d", errno);
        return {ESP_FAIL, std::nullopt};
    }

    if (from != nullptr) {
        *from = sender;
    }

    buffer.resize(len);
    return {ESP_OK, buffer};
}