#pragma once

#include "config/ConfigStore.h"

#include <algorithm>
#include <map>

namespace ewfm {

class MemoryConfigStorage final : public IConfigStorage {
public:
    bool begin(const char* namespaceName, bool readOnly) override {
        (void)namespaceName;
        readOnly_ = readOnly;
        return true;
    }
    void end() override {}
    bool hasKey(const char* key) const override {
        return strings_.count(key) || blobs_.count(key) || uints_.count(key) || bools_.count(key);
    }
    bool putString(const char* key, const std::string& value) override {
        if (readOnly_) {
            return false;
        }
        blobs_.erase(key);
        uints_.erase(key);
        bools_.erase(key);
        strings_[key] = value;
        return true;
    }
    bool getString(const char* key, std::string& value) const override {
        const auto it = strings_.find(key);
        if (it == strings_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }
    bool putBlob(const char* key, const uint8_t* value, size_t size) override {
        if (readOnly_) {
            return false;
        }
        strings_.erase(key);
        uints_.erase(key);
        bools_.erase(key);
        blobs_[key] = std::vector<uint8_t>(value, value + size);
        return true;
    }
    bool getBlob(const char* key, uint8_t* value, size_t& size) const override {
        const auto it = blobs_.find(key);
        if (it == blobs_.end() || size < it->second.size()) {
            return false;
        }
        std::copy(it->second.begin(), it->second.end(), value);
        size = it->second.size();
        return true;
    }
    bool putBlob(const char* key, const std::vector<uint8_t>& value) override {
        return putBlob(key, value.data(), value.size());
    }
    bool getBlob(const char* key, std::vector<uint8_t>& value) const override {
        const auto it = blobs_.find(key);
        if (it == blobs_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }
    bool putUInt(const char* key, uint32_t value) override {
        if (readOnly_) {
            return false;
        }
        strings_.erase(key);
        blobs_.erase(key);
        bools_.erase(key);
        uints_[key] = value;
        return true;
    }
    bool getUInt(const char* key, uint32_t& value) const override {
        const auto it = uints_.find(key);
        if (it == uints_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }
    bool putBool(const char* key, bool value) override {
        if (readOnly_) {
            return false;
        }
        strings_.erase(key);
        blobs_.erase(key);
        uints_.erase(key);
        bools_[key] = value;
        return true;
    }
    bool getBool(const char* key, bool& value) const override {
        const auto it = bools_.find(key);
        if (it == bools_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }
    bool remove(const char* key) override {
        if (readOnly_) {
            return false;
        }
        strings_.erase(key);
        blobs_.erase(key);
        uints_.erase(key);
        bools_.erase(key);
        return true;
    }
    bool clear() override {
        if (readOnly_) {
            return false;
        }
        strings_.clear();
        blobs_.clear();
        uints_.clear();
        bools_.clear();
        return true;
    }

private:
    bool readOnly_{false};
    std::map<std::string, std::string> strings_;
    std::map<std::string, std::vector<uint8_t>> blobs_;
    std::map<std::string, uint32_t> uints_;
    std::map<std::string, bool> bools_;
};

} // namespace ewfm
