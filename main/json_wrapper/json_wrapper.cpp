#include "json_wrapper.hpp"
JsonWrapper::JsonWrapper(const std::string& str) {
    json_ = cJSON_Parse(str.c_str());
    std::cout << "aboba constructor 1 invoked!" << std::endl;
    if (json_ == nullptr) {
        throw std::runtime_error("Failed to parse JSON string");
    }
}
JsonWrapper::JsonWrapper(const char* str) {
    json_ = cJSON_Parse(str);
    std::cout << "aboba constructor 2 invoked!" << std::endl;
    if (json_ == nullptr) {
        throw std::runtime_error("Failed to parse JSON string");
    }
}
JsonWrapper::JsonWrapper(const uint8_t* str) {
    json_ = cJSON_Parse(reinterpret_cast<const char*>(str));
    std::cout << "aboba constructor 3 invoked!" << std::endl;
    if (json_ == nullptr) {
        throw std::runtime_error("Failed to parse JSON string");
    }
}
// Constructor: Initializes a cJSON object (default is an object)
JsonWrapper::JsonWrapper() {
    json_ = cJSON_CreateObject();
}

// Destructor: Frees the cJSON object
JsonWrapper::~JsonWrapper() {
    if (json_) {
        cJSON_Delete(json_);
    }
}

// Disable copy constructor and assignment to prevent double-free
JsonWrapper::JsonWrapper(const JsonWrapper& other) {
    if (other.json_) {
        json_ = cJSON_Duplicate(other.json_, cJSON_True);
    }
}

// Move assignment
JsonWrapper& JsonWrapper::operator=(JsonWrapper&& other) noexcept {
    std::cout << "aboba move assignmed operator  invoked!" << std::endl;
    if (this != &other) {
        if (json_) {
            cJSON_Delete(json_);
        }
        json_ = other.json_;
        other.json_ = nullptr;
    }
    return *this;
}

// Add a boolean to the JSON object
void JsonWrapper::addBool(const char* key, bool value) {
#ifdef ENABLE_SAFENET
    if (json_) {
#endif
        cJSON_AddBoolToObject(json_, key, value);
#ifdef ENABLE_SAFENET
    } else {
        throw std::runtime_error("cJSON object is null!");
    }
#endif
}

// Add a number (integer) to the JSON object
void JsonWrapper::addNumber(const char* key, int value) {
#ifdef ENABLE_SAFENET
    if (json_) {
#endif
        cJSON_AddNumberToObject(json_, key, value);
#ifdef ENABLE_SAFENET
    } else {
        throw std::runtime_error("cJSON object is null!");
    }
#endif
}

// Add a string to the JSON object
void JsonWrapper::addString(const char* key, const std::string& str) {
#ifdef ENABLE_SAFENET
    if (json_) {
#endif
        cJSON_AddStringToObject(json_, key, str.c_str());
#ifdef ENABLE_SAFENET
    } else {
        throw std::runtime_error("cJSON object is null!");
    }
#endif
}

void JsonWrapper::addFloat(const char* key, float value) {
#ifdef ENABLE_SAFENET
    if (json_) {
#endif
        cJSON_AddNumberToObject(json_, key, value);
#ifdef ENABLE_SAFENET
    } else {
        throw std::runtime_error("cJSON object is null!");
    }
#endif
}

// New read methods
std::optional<bool> JsonWrapper::getBool(const char* key) const {
    cJSON* item = cJSON_GetObjectItem(json_, key);
    if (!item) {
        return std::nullopt;
    }
    return cJSON_IsTrue(item);
}

std::optional<int> JsonWrapper::getNumber(const char* key) const {
    cJSON* item = cJSON_GetObjectItem(json_, key);
    if (!item || !cJSON_IsNumber(item)) {
        esp_log(
          kErrorLogConfig,
          "JSON_WRAPPER",
          "getNumber: item is null or not a number: %d %d",
          item == nullptr,
          cJSON_IsNumber(item));
        return std::nullopt;
    }
    return static_cast<int>(item->valuedouble);
}

std::optional<float> JsonWrapper::getFloat(const char* key) const {
    cJSON* item = cJSON_GetObjectItem(json_, key);
    if (!item || !cJSON_IsNumber(item)) {
        return std::nullopt;
    }
    return static_cast<float>(item->valuedouble);
}

std::optional<std::string> JsonWrapper::getString(const char* key) const {
    cJSON* item = cJSON_GetObjectItem(json_, key);
    if (!item || !cJSON_IsString(item) || !item->valuestring) {
        return std::nullopt;
    }
    return std::string(item->valuestring);
}

// Serialize to string (caller must free the returned pointer)
std::string JsonWrapper::toString() const {
#ifdef ENABLE_SAFENET
    if (json_ == nullptr) {
        throw std::runtime_error("cJSON object is null!");
    }
#endif
    std::string raw_val = cJSON_PrintUnformatted(json_);
    return raw_val;
}

char* JsonWrapper::serialize() const {
    return cJSON_PrintUnformatted(json_);
}

// Get the underlying cJSON pointer (use with caution)
const cJSON* JsonWrapper::getJson() const {
    return json_;
}

JsonWrapper& JsonWrapper::operator=(const JsonWrapper& other) {
    if (&other == this) {
        return *this;
    }
    if (json_) {
        cJSON_Delete(json_);
    }
    if (other.json_) {
        json_ = cJSON_Duplicate(other.json_, cJSON_True);
    }

    return *this;
}

// Move constructor
JsonWrapper::JsonWrapper(JsonWrapper&& other) noexcept : json_(other.json_) {
    other.json_ = nullptr;
    std::cout << "aboba move constructor invoked!" << std::endl;
}
