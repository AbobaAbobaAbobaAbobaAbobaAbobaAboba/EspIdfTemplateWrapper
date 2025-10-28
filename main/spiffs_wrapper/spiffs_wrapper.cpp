#include "spiffs_wrapper.hpp"
#include "../common/common.hpp"
namespace SPIFFS {
void SpiffsWrapper::printFiles() {
#ifdef ENABLE_SAFENET
    if (is_mounted_ == false) {
        throw std::runtime_error("SPIFFS not mounted!");
    }
#endif

    esp_log(kInfoLogConfig, "SPIFFS", "Listing files in SPIFFS: \n");
    DIR* dir = opendir("/spiffs");
    if (dir) {
        struct dirent* de;
        while ((de = readdir(dir)) != NULL) {
            esp_log(kInfoLogConfig, "SPIFFS", "Found: %s \n", de->d_name);
        }
        closedir(dir);
    }
}
[[nodiscard]] std::vector<std::string> SpiffsWrapper::getPaths() {
    std::vector<std::string> paths;
#ifdef ENABLE_SAFENET
    if (is_mounted_ == false) {
        throw std::runtime_error("SPIFFS not mounted!");
    }
#endif
    DIR* dir = opendir("/spiffs");
    if (dir) {
        struct dirent* de;
        while ((de = readdir(dir)) != NULL) {
            paths.push_back(de->d_name);
            std::string filename = de->d_name;
            esp_log(
              kInfoLogConfig,
              "SPIFFS",
              "Found path: %s \n",
              ("/spiffs/" + filename).c_str());
        }
        closedir(dir);
    }
    return paths;
}

std::tuple<ErrorCode, const char*, size_t> SpiffsWrapper::readFileChunked(
  const std::string& filepath) {
#ifdef ENABLE_SAFENET
    if (is_mounted_ == false) {
        throw std::runtime_error("SPIFFS not mounted!");
    }
#endif

    static std::array<char, 256> buffer;

    static FILE* fd = NULL;

    std::string full_path = "/spiffs" + filepath;

    if (fd == NULL) {
        fd = fopen(full_path.c_str(), "r");
    }

    if (fd == NULL) {
        esp_log(
          kErrorLogConfig,
          "SPIFFS",
          "Requested path not found: %s \n",
          full_path.c_str());

        return {ErrorCode::FILE_NOT_FOUND, buffer.data(), 0};
    }

    size_t read_len;
    read_len = fread(buffer.data(), 1, buffer.size(), fd);

    if (read_len < buffer.size()) {
        fclose(fd);
        fd = NULL;
        return {ErrorCode::FINISHED_READ, buffer.data(), read_len};
    } else {
        return {ErrorCode::CONTINUE_READ, buffer.data(), buffer.size()};
    }
}
void SpiffsWrapper::printFilesystemInfo() {
#ifdef ENABLE_SAFENET
    if (is_mounted_ == false) {
        throw std::runtime_error("SPIFFS not mounted!");
    }
#endif
    size_t total = 0, used = 0;
    esp_spiffs_info("spiffs", &total, &used);
    esp_log(
      kInfoLogConfig,
      "SPIFFS",
      "Filesystem info: total %d, used %d \n",
      total,
      used);
}

void SpiffsWrapper::init() {
    if (!is_mounted_) {
        if (!mountPrivate()) {
            throw std::runtime_error("Failed to mount SPIFFS filesystem!");
        }
    }
}

[[nodiscard]] bool SpiffsWrapper::mountPrivate() {
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret == ESP_OK) {
        is_mounted_ = true;
        return true;
    }
    if (ret == ESP_FAIL) {
        esp_log(kErrorLogConfig, "SPIFFS", "Failed to mount or format SPIFFS");
    } else if (ret == ESP_ERR_NOT_FOUND) {
        esp_log(kErrorLogConfig, "SPIFFS", "Failed to find SPIFFS partition");
    } else {
        esp_log(
          kErrorLogConfig,
          "SPIFFS",
          "Failed to initialize SPIFFS (%s)",
          esp_err_to_name(ret));
    }
    is_mounted_ = false;
    return false;
}
}  // namespace SPIFFS