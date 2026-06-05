#pragma once

#include "config/ConfigStore.h"

#include <map>

namespace ewfm {

class MemoryConfigStorage final : public IConfigStorage {
public:
    bool begin(const char* ns, bool readOnly) override {
        (void)ns;
        readOnly_ = readOnly;
        return true;
    }
    void end() override {}
    bool hasKey(const char* key) const override {
        return strings_.count(key) || uints_.count(key) || bools_.count(key);
    }
    bool putString(const char* key, const std::string& value) override {
        if (readOnly_)
            return false;
        strings_[key] = value;
        return true;
    }
    bool getString(const char* key, std::string& value) const override {
        auto it = strings_.find(key);
        if (it == strings_.end())
            return false;
        value = it->second;
        return true;
    }
    bool putUInt(const char* key, uint32_t value) override {
        if (readOnly_)
            return false;
        uints_[key] = value;
        return true;
    }
    bool getUInt(const char* key, uint32_t& value) const override {
        auto it = uints_.find(key);
        if (it == uints_.end())
            return false;
        value = it->second;
        return true;
    }
    bool putBool(const char* key, bool value) override {
        if (readOnly_)
            return false;
        bools_[key] = value;
        return true;
    }
    bool getBool(const char* key, bool& value) const override {
        auto it = bools_.find(key);
        if (it == bools_.end())
            return false;
        value = it->second;
        return true;
    }
    bool remove(const char* key) override {
        if (readOnly_)
            return false;
        strings_.erase(key);
        uints_.erase(key);
        bools_.erase(key);
        return true;
    }

private:
    bool readOnly_{false};
    std::map<std::string, std::string> strings_;
    std::map<std::string, uint32_t> uints_;
    std::map<std::string, bool> bools_;
};

} // namespace ewfm
