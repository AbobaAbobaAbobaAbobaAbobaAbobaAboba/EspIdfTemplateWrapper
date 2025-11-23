#ifndef CONFIG_WRAPPER_HPP
#define CONFIG_WRAPPER_HPP

#pragma once

#include "../littlefs_wrapper/littlefs_wrapper.hpp"
#include "ini.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
// Forward declaration if needed
class LittleFsWrapper;
struct Section {
    std::map<std::string, std::string> keyvalues;
};
class ConfigWrapper {
  public:
    explicit ConfigWrapper(const std::string& filename) : filename_(filename) {
        load();
    }

    void load() {
        sections_.clear();
        std::string content = readFullFile();
        if (content.empty()) return;

        int result = ini_parse_string(content.c_str(), ini_handler, this);
        if (result != 0) {
            ESP_LOGE("Config", "Failed to parse INI: error at line %d", result);
            throw std::runtime_error("INI parse error");
        }
    }

    void save() const {
        std::string ini_data = serialize();
        std::vector<char> buffer(ini_data.begin(), ini_data.end());
        LittleFS::LittleFsWrapper::write(filename_, buffer);
        ESP_LOGI("Config", "Config saved to %s", filename_.c_str());
    }

    // --- Getters ---
    template <typename T>
    T get(const std::string& section, const std::string& key, T default_val = T{})
      const;

    // --- Setters ---

    template <typename T>
    void set(const std::string& section, const std::string& key, T value) {
        if constexpr (std::is_same_v<T, std::string>) {
            set_private(section, key, value);
        } else {
            set_private(section, key, std::to_string(value));
        }
    }

    bool hasSection(const std::string& section) const;

    bool hasKey(const std::string& section, const std::string& key) const;

    void removeSection(const std::string& section);

    void removeKey(const std::string& section, const std::string& key);
    std::map<std::string, Section> getSections() const;
    void printContents();

  private:
    inline static constexpr TagType TAG = "CONFIG_WRAPPER";

    std::map<std::string, Section> sections_;
    std::string filename_;

    static int
    ini_handler(void* user, const char* section, const char* name, const char* value);

    std::string readFullFile();

    std::string serialize() const;

    void set_private(
      const std::string& section,
      const std::string& key,
      const std::string& value);
};

template <>
inline std::string ConfigWrapper::get(
  const std::string& section,
  const std::string& key,
  std::string default_val) const {
    auto sec_it = sections_.find(section);
    if (sec_it == sections_.end()) return default_val;
    auto kv_it = sec_it->second.keyvalues.find(key);
    return kv_it != sec_it->second.keyvalues.end() ? kv_it->second : default_val;
}

template <>
inline int ConfigWrapper::get(
  const std::string& section,
  const std::string& key,
  int default_val) const {
    const std::string& val = get<std::string>(section, key);
    if (val.empty()) return default_val;
    try {
        return std::stoi(val);
    } catch (...) {
        return default_val;
    }
}

template <>
inline float ConfigWrapper::get(
  const std::string& section,
  const std::string& key,
  float default_val) const {
    const std::string& val = get<std::string>(section, key);
    if (val.empty()) return default_val;
    try {
        return std::stof(val);
    } catch (...) {
        return default_val;
    }
}

template <>
inline double ConfigWrapper::get(
  const std::string& section,
  const std::string& key,
  double default_val) const {
    const std::string& val = get<std::string>(section, key);
    if (val.empty()) return default_val;
    try {
        return std::stod(val);
    } catch (...) {
        return default_val;
    }
}

template <>
inline bool ConfigWrapper::get(
  const std::string& section,
  const std::string& key,
  bool default_val) const {
    const std::string& val = get<std::string>(section, key);
    if (val.empty()) return default_val;
    std::string lower = val;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") return true;
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off")
        return false;
    return default_val;
}

#endif  // CONFIG_WRAPPER_HPP