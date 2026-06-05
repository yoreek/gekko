#include "config/ConfigStore.h"

#include "debug/Debug.h"

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device";
constexpr const char* kSchema = "schema";
constexpr const char* kDevice = "device";
constexpr const char* kSsid = "ssid";
constexpr const char* kPassword = "password";
constexpr const char* kHttpPortal = "http_portal";
constexpr const char* kMobileProv = "mobile_prov";
constexpr const char* kWebOta = "web_ota";
} // namespace

bool ConfigStore::begin() {
    return storage_.begin(kNamespace, false);
}

ValidationResult ConfigStore::load() {
    config_ = defaultConfig();

    uint32_t schema = config_.schemaVersion;
    if (storage_.getUInt(kSchema, schema)) {
        config_.schemaVersion = schema;
    }
    storage_.getString(kDevice, config_.deviceName);
    storage_.getString(kSsid, config_.wifi.ssid);
    storage_.getString(kPassword, config_.wifi.password);
    storage_.getBool(kHttpPortal, config_.provisioning.httpPortalEnabled);
    storage_.getBool(kMobileProv, config_.provisioning.mobileProvisioningEnabled);
    storage_.getBool(kWebOta, config_.firmwareUpdate.webOtaEnabled);

    ValidationResult migrated = migrateConfig(config_);
    if (!migrated.ok()) {
        config_ = defaultConfig();
        return migrated;
    }

    if (config_.schemaVersion == kCurrentConfigSchemaVersion) {
        save(config_);
    }
    return {};
}

ValidationResult ConfigStore::save(const DeviceConfig& config) {
    ValidationResult validation = validateConfig(config, false);
    if (!validation.ok()) {
        return validation;
    }

    bool ok = storage_.putUInt(kSchema, config.schemaVersion) && storage_.putString(kDevice, config.deviceName) &&
              storage_.putString(kSsid, config.wifi.ssid) && storage_.putString(kPassword, config.wifi.password) &&
              storage_.putBool(kHttpPortal, config.provisioning.httpPortalEnabled) &&
              storage_.putBool(kMobileProv, config.provisioning.mobileProvisioningEnabled) &&
              storage_.putBool(kWebOta, config.firmwareUpdate.webOtaEnabled);

    if (!ok) {
        EWFM_CONFIG_LOG_WARN("failed to save configuration");
        return {ConfigError::StorageError, "failed to save configuration"};
    }

    config_ = config;
    EWFM_CONFIG_LOG_INFO("configuration saved");
    return {};
}

ValidationResult ConfigStore::saveWifiCredentials(const WiFiCredentials& credentials) {
    ValidationResult validation = validateWifiCredentials(credentials, true);
    if (!validation.ok()) {
        return validation;
    }
    DeviceConfig next = config_;
    next.wifi = credentials;
    return save(next);
}

bool ConfigStore::clearWifiCredentials() {
    config_.wifi = {};
    bool removed = storage_.remove(kSsid);
    removed = storage_.remove(kPassword) && removed;
    return removed && save(config_).ok();
}

} // namespace ewfm
