#ifndef JSON_WRAPPER_HPP
#define JSON_WRAPPER_HPP

#include "../common/common.hpp"
#include <cJSON.h>
#include <exception>
#include <memory>
#include <optional>
#include <string>
// Functor as deleter
struct CJsonFree {
    void operator()(char* p) const {
        free(p);  // free memory allocated by cJSON_PrintUnformatted
    }
};

class JsonWrapper {
  public:
    JsonWrapper(const std::string& str);
    JsonWrapper(const char* str);
    JsonWrapper(const uint8_t* str);
    JsonWrapper();
    ~JsonWrapper();
    JsonWrapper(const JsonWrapper& other);
    JsonWrapper& operator=(const JsonWrapper& other);
    JsonWrapper(JsonWrapper&& other) noexcept;
    JsonWrapper& operator=(JsonWrapper&& other) noexcept;

    void addBool(const char* key, bool value);
    void addNumber(const char* key, int value);
    void addString(const char* key, const std::string& str);
    void addFloat(const char* key, float value);

    void setBool(const char* key, bool value) {
#ifdef ENABLE_SAFENET
        if (json_) {
#endif
            cJSON_ReplaceItemInObject(json_, key, cJSON_CreateBool(value));
#ifdef ENABLE_SAFENET
        } else {
            throw std::runtime_error("cJSON object is null!");
        }
#endif
    }

    void setNumber(const char* key, int value) {
#ifdef ENABLE_SAFENET
        if (json_) {
#endif
            cJSON_ReplaceItemInObject(json_, key, cJSON_CreateNumber(value));
#ifdef ENABLE_SAFENET
        } else {
            throw std::runtime_error("cJSON object is null!");
        }
#endif
    }
    void setFloat(const char* key, float value) {
#ifdef ENABLE_SAFENET
        if (json_) {
#endif
            cJSON_ReplaceItemInObject(json_, key, cJSON_CreateNumber(value));
#ifdef ENABLE_SAFENET
        } else {
            throw std::runtime_error("cJSON object is null!");
        }
#endif
    }
    void setString(const char* key, const std::string& value) {
#ifdef ENABLE_SAFENET
        if (json_) {
#endif
            cJSON_ReplaceItemInObject(json_, key, cJSON_CreateString(value.c_str()));
#ifdef ENABLE_SAFENET
        } else {
            throw std::runtime_error("cJSON object is null!");
        }
#endif
    }
    std::optional<bool> getBool(const char* key) const;
    std::optional<int> getNumber(const char* key) const;
    std::optional<float> getFloat(const char* key) const;
    std::optional<std::string> getString(const char* key) const;

    std::shared_ptr<char> serialize() const;
    const cJSON* getJson() const;

  private:
    cJSON* json_ = nullptr;
    inline static int counter = 0;
};

#endif  // JSON_WRAPPER_HPP