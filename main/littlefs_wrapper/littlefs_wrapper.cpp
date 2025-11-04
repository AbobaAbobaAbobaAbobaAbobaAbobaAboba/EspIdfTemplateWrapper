#include "littlefs_wrapper.hpp"
#include "../common/common.hpp"
#include <stack>
namespace LittleFS {

void LittleFsWrapper::printFiles() {
#ifdef ENABLE_SAFENET
    if (!is_mounted_) {
        throw std::runtime_error("LittleFS not mounted!");
    }
#endif

    ESP_LOGI("LittleFS", "Listing files in LittleFS:");

    DIR* dir = opendir("/littlefs");
    if (!dir) {
        ESP_LOGE("LittleFS", "Failed to open directory /littlefs");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        ESP_LOGI("LittleFS", "Found: %s", entry->d_name);
    }
    closedir(dir);
}

std::vector<std::string> LittleFsWrapper::getPaths() {
    std::vector<std::string> paths;

#ifdef ENABLE_SAFENET
    if (!is_mounted_) {
        throw std::runtime_error("LittleFS not mounted!");
    }
#endif

    std::stack<std::string> dirs;
    dirs.push("");  // Start from root (empty prefix)

    while (!dirs.empty()) {
        std::string current_dir = dirs.top();
        dirs.pop();

        std::string full_dir_path = "/littlefs" + current_dir;
        DIR* dir = opendir(full_dir_path.c_str());

        if (!dir) {
            ESP_LOGW("LittleFS", "Failed to open directory: %s", full_dir_path.c_str());
            continue;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                std::string name = entry->d_name;
                if (name == "." || name == "..") continue;

                // Build subdir path: "/subdir" or "/parent/child"
                std::string subdir_path =
                  current_dir.empty() ? "/" + name : current_dir + "/" + name;

                dirs.push(subdir_path);
                ESP_LOGI("LittleFS", "Found directory: %s", subdir_path.c_str());
            } else if (entry->d_type == DT_REG) {
                // Regular file
                std::string rel_path = current_dir.empty()
                                         ? "/" + std::string(entry->d_name)
                                         : current_dir + "/" + entry->d_name;

                paths.push_back(rel_path);
                ESP_LOGI("LittleFS", "Found file: %s", rel_path.c_str());
            }
        }
        closedir(dir);
    }

    return paths;
}

std::tuple<ErrorCode, const char*, size_t> LittleFsWrapper::readFileChunked(
  const std::string& filepath) {
#ifdef ENABLE_SAFENET
    if (!is_mounted_) {
        throw std::runtime_error("LittleFS not mounted!");
    }
#endif

    static std::array<char, 256> buffer{};
    static FILE* fd = nullptr;

    const std::string full_path = "/littlefs" + filepath;

    // Open file on first call
    if (!fd) {
        fd = fopen(full_path.c_str(), "rb");
        if (!fd) {
            ESP_LOGE("LittleFS", "File not found: %s", full_path.c_str());
            return {ErrorCode::FILE_NOT_FOUND, buffer.data(), 0};
        }
    }

    size_t bytes_read = fread(buffer.data(), 1, buffer.size(), fd);

    if (bytes_read < buffer.size() || feof(fd)) {
        fclose(fd);
        fd = nullptr;
        return {ErrorCode::FINISHED_READ, buffer.data(), bytes_read};
    }

    return {ErrorCode::CONTINUE_READ, buffer.data(), bytes_read};
}

void LittleFsWrapper::printFilesystemInfo() {
#ifdef ENABLE_SAFENET
    if (!is_mounted_) {
        throw std::runtime_error("LittleFS not mounted!");
    }
#endif

    size_t total = 0, used = 0;
    esp_err_t ret = esp_littlefs_info("littlefs", &total, &used);

    if (ret == ESP_OK) {
        ESP_LOGI(
          "LittleFS",
          "Filesystem info: total %zu bytes, used %zu bytes",
          total,
          used);
    } else {
        ESP_LOGE("LittleFS", "Failed to get filesystem info: %s", esp_err_to_name(ret));
    }
}

void LittleFsWrapper::init() {
    if (!is_mounted_) {
        if (!mountPrivate()) {
            throw std::runtime_error("Failed to mount LittleFS filesystem!");
        }
    }
}

bool LittleFsWrapper::mountPrivate() {
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret == ESP_OK) {
        is_mounted_ = true;
        ESP_LOGI("LittleFS", "LittleFS mounted successfully at %s", conf.base_path);
        return true;
    }

    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGE("LittleFS", "Failed to find LittleFS partition. Check partitions.csv");
    } else if (ret == ESP_FAIL) {
        ESP_LOGE("LittleFS", "Failed to mount or format LittleFS");
    } else {
        ESP_LOGE(
          "LittleFS",
          "Failed to initialize LittleFS (%s)",
          esp_err_to_name(ret));
    }

    is_mounted_ = false;
    return false;
}

}  // namespace LittleFS