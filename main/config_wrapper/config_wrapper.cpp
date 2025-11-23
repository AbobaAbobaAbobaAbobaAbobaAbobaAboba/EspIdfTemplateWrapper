#include "config_wrapper.hpp"

bool ConfigWrapper::hasSection(const std::string& section) const {
    return sections_.find(section) != sections_.end();
}
bool ConfigWrapper::hasKey(const std::string& section, const std::string& key) const {
    auto sec_it = sections_.find(section);
    return sec_it != sections_.end()
           && sec_it->second.keyvalues.find(key) != sec_it->second.keyvalues.end();
}
void ConfigWrapper::removeSection(const std::string& section) {
    sections_.erase(section);
}
void ConfigWrapper::removeKey(const std::string& section, const std::string& key) {
    auto it = sections_.find(section);
    if (it != sections_.end()) {
        it->second.keyvalues.erase(key);
        if (it->second.keyvalues.empty()) {
            sections_.erase(it);
        }
    }
}
std::map<std::string, Section> ConfigWrapper::getSections() const {
    return sections_;
}
void ConfigWrapper::printContents() {
    for (auto it = sections_.begin(), ie = sections_.end(); it != ie; ++it) {
        esp_log(kInfoLogConfig, TAG, "[%s]", it->first.c_str());

        for (auto jt = it->second.keyvalues.begin(), je = it->second.keyvalues.end();
             jt != je;
             ++jt) {
            esp_log(
              kInfoLogConfig,
              TAG,
              "%s: %s",
              jt->first.c_str(),
              jt->second.c_str());
        }
    }
}
int ConfigWrapper::ini_handler(
  void* user,
  const char* section,
  const char* name,
  const char* value) {
    auto* config = static_cast<ConfigWrapper*>(user);
    config->sections_[section].keyvalues[name] = value;
    return 1;
}

// Helper to convert buffer chunks into full string
std::string ConfigWrapper::readFullFile() {
    std::string content;
    content.reserve(1024);
    auto file_descriptor = LittleFS::LittleFsWrapper::open(filename_);
    while (true) {
        auto [err, data, size] = file_descriptor.readFileChunked();

        if (size > 0) {
            content.append(data, size);
        }
        if (err == LittleFS::ErrorCode::FINISHED_READ) break;
        if (err == LittleFS::ErrorCode::FILE_NOT_FOUND) {
            esp_log(
              kErrorLogConfig,
              TAG,
              "Config file not found: %s, starting fresh",
              filename_.c_str());
            return "";
        }
    }
    return content;
}

// Serialize current config to INI string
std::string ConfigWrapper::serialize() const {
    std::string out;
    out.reserve(1024);

    for (const auto& [sec_name, section] : sections_) {
        out += '[';
        out += sec_name;
        out += "]\n";

        for (const auto& [key, val] : section.keyvalues) {
            out += key;
            out += '=';
            out += val;
            out += '\n';
        }
        out += '\n';
    }
    return out;
}

void ConfigWrapper::set_private(
  const std::string& section,
  const std::string& key,
  const std::string& value) {
    sections_[section].keyvalues[key] = value;
}
