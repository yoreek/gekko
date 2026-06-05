#pragma once

#include "config/DeviceConfig.h"

#include <cstdint>
#include <string>

namespace ewfm {

class IConfigStorage {
public:
    virtual ~IConfigStorage() = default;
    virtual bool begin(const char* ns, bool readOnly) = 0;
    virtual void end() = 0;
    virtual bool hasKey(const char* key) const = 0;
    virtual bool putString(const char* key, const std::string& value) = 0;
    virtual bool getString(const char* key, std::string& value) const = 0;
    virtual bool putUInt(const char* key, uint32_t value) = 0;
    virtual bool getUInt(const char* key, uint32_t& value) const = 0;
    virtual bool putBool(const char* key, bool value) = 0;
    virtual bool getBool(const char* key, bool& value) const = 0;
    virtual bool remove(const char* key) = 0;
};

class ConfigStore {
public:
    explicit ConfigStore(IConfigStorage& storage) : storage_(storage) {}

    bool begin();
    const DeviceConfig& config() const {
        return config_;
    }
    DeviceConfig& mutableConfig() {
        return config_;
    }

    ValidationResult load();
    ValidationResult save(const DeviceConfig& config);
    ValidationResult saveWifiCredentials(const WiFiCredentials& credentials);
    bool clearWifiCredentials();

private:
    IConfigStorage& storage_;
    DeviceConfig config_;
};

} // namespace ewfm
