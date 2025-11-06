#ifndef LITTLEFS_WRAPPER_HPP
#define LITTLEFS_WRAPPER_HPP
#include "../common/common.hpp"
#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <array>
#include <atomic>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace LittleFS {

enum class ErrorCode : uint8_t {
    OK,
    MOUNT_FAILED,
    PARTITION_NOT_FOUND,
    CONTINUE_READ,
    FINISHED_READ,
    FILE_NOT_FOUND
};

class ReadDescriptor {
  public:
    ReadDescriptor() = delete;
    ReadDescriptor(const ReadDescriptor& other) = delete;
    ReadDescriptor(ReadDescriptor&& other) = delete;
    ReadDescriptor& operator=(const ReadDescriptor& other) = delete;
    ReadDescriptor& operator=(ReadDescriptor&& other) = delete;
    ReadDescriptor(FILE* fd);

    ~ReadDescriptor();
    std::tuple<ErrorCode, const char*, size_t> readFileChunked();
    bool isValid();
    void close();

  private:
    std::array<char, 256> buffer_{};
    FILE* fd_ = nullptr;
    inline static constexpr TagType TAG = "READ_DESCRIPTOR";
};

class LittleFsWrapper {
  public:
    static void printFiles();
    static std::vector<std::string> getPaths();

    static ReadDescriptor open(const std::string& filepath) {
#ifdef ENABLE_SAFENET
        if (!LittleFsWrapper::is_mounted_) {
            throw std::runtime_error("LittleFS not mounted!");
        }

#endif
        std::string full_path = "/littlefs" + filepath;
        FILE* fd = fopen(full_path.c_str(), "rb");
        return {fd};
    }

    static void printFilesystemInfo();
    static void init();
    static void write(const std::string& filename, const std::vector<char>& data);

  private:
    friend class ReadDescriptor;
    static bool mountPrivate();

    inline static esp_vfs_littlefs_conf_t conf = {
      .base_path = "/littlefs",
      .partition_label = "littlefs",
      .partition = NULL,
      .format_if_mount_failed = true,
      .read_only = false,
      .dont_mount = false,
      .grow_on_mount = true};

    inline static bool is_mounted_ = false;
    inline static constexpr TagType TAG = "LITTLEFS";
};

}  // namespace LittleFS

#endif  // LITTLEFS_WRAPPER_HPP