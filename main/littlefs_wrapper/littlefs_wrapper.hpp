#ifndef LITTLEFS_WRAPPER_HPP
#define LITTLEFS_WRAPPER_HPP

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

class LittleFsWrapper {
  public:
    static void printFiles();
    static std::vector<std::string> getPaths();

    static std::tuple<ErrorCode, const char*, size_t> readFileChunked(
      const std::string& filepath);

    static void printFilesystemInfo();
    static void init();

  private:
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
};

}  // namespace LittleFS

#endif  // LITTLEFS_WRAPPER_HPP