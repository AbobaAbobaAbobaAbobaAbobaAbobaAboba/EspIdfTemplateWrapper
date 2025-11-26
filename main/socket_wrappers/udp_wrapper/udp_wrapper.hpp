#ifndef UDP_WRAPPER_HPP
#define UDP_WRAPPER_HPP

#include "../../common/common.hpp"
#include <arpa/inet.h>
#include <esp_err.h>
#include <esp_log.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

class UdpWrapper {
  public:
    UdpWrapper();
    ~UdpWrapper();

    UdpWrapper(const UdpWrapper& other) = delete;
    UdpWrapper(UdpWrapper&& other) = delete;

    UdpWrapper& operator=(const UdpWrapper& other) = delete;
    UdpWrapper& operator=(UdpWrapper&& other) = delete;

    bool isLocked() { return is_locked_; }

    [[nodiscard]] esp_err_t lock(
      uint16_t local_port = 0,
      const std::string& local_ip = "");

    esp_err_t unlock();

    esp_err_t send(const std::string& address, uint16_t port, std::string data);

    esp_err_t
    send(const std::string& address, uint16_t port, const uint8_t* data, size_t len);

    esp_err_t
    send(const std::string& address, uint16_t port, const std::vector<uint8_t>& data);

    [[nodiscard]] std::tuple<esp_err_t, std::optional<std::vector<uint8_t>>> recv(
      sockaddr_in* from,  // Can be nullptr if sender address is not needed
      size_t max_len,
      int timeout_ms =
        -1);  // -1 for blocking, 0 for non-blocking, >0 for timeout in ms

  private:
    int sock_;
    uint16_t local_port_;
    std::string local_ip_;
    bool is_locked_ = false;

    inline static constexpr TagType TAG = "UDP_WRAPPER";
};

#endif