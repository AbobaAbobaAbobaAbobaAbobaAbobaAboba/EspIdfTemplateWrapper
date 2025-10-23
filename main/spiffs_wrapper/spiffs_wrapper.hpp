#ifndef SPIFFS_WRAPPER_HPP
#define SPIFFS_WRAPPER_HPP
#include "../common/common.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <array>
#include <atomic>
#include <exception>
#include <string>
#include <tuple>
#include <vector>
namespace SPIFFS {
enum class ErrorCode : uint8_t {
    OK,
    MOUNT_FAILED,
    PARTITION_NOT_FOUND,
    CONTINUE_READ,
    FINISHED_READ,
    FILE_NOT_FOUND
};
class SpiffsWrapper {
  public:
    static void printFiles();
    [[nodiscard]] static std::vector<std::string> getPaths();

    static std::tuple<ErrorCode, const char*, size_t> readFileChunked(
      const std::string& filepath);

    static void printFilesystemInfo();
    static void init();

  private:
    [[nodiscard]] static bool mountPrivate();

    inline static esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",         // Mount point (prefix for all files)
      .partition_label = "spiffs",    // Matches partitions.csv
      .max_files = 5,                 // Max open files
      .format_if_mount_failed = true  // Format on mount fail (dev only!)
    };
    inline static bool is_mounted_ = false;
};
};      // namespace SPIFFS
#endif  // SPIFFS_WRAPPER_HPP
