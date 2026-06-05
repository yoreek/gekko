#include "config/DeviceConfig.h"

namespace ewfm {

DeviceConfig defaultConfig() {
    return DeviceConfig{};
}

ValidationResult validateWifiCredentials(const WiFiCredentials& credentials, bool requireCredentials) {
    if (requireCredentials && credentials.ssid.empty()) {
        return {ConfigError::EmptySsid, "ssid is required"};
    }
    if (credentials.ssid.size() > kMaxSsidLength) {
        return {ConfigError::SsidTooLong, "ssid is too long"};
    }
    if (credentials.password.size() > kMaxPasswordLength) {
        return {ConfigError::PasswordTooLong, "password is too long"};
    }
    return {};
}

ValidationResult validateConfig(const DeviceConfig& config, bool requireWifiCredentials) {
    if (config.schemaVersion > kCurrentConfigSchemaVersion || config.schemaVersion == 0) {
        return {ConfigError::UnsupportedSchemaVersion, "unsupported schema version"};
    }
    if (config.deviceName.empty()) {
        return {ConfigError::DeviceNameEmpty, "device name is required"};
    }
    if (config.deviceName.size() > kMaxDeviceNameLength) {
        return {ConfigError::DeviceNameTooLong, "device name is too long"};
    }
    if (config.maxJsonBytes == 0 || config.maxJsonBytes > 8192) {
        return {ConfigError::JsonTooLarge, "json limit is invalid"};
    }
    if (config.provisioning.proofOfPossession.size() > 64 || config.provisioning.serviceKey.size() > 64) {
        return {ConfigError::PasswordTooLong, "provisioning secret is too long"};
    }
    return validateWifiCredentials(config.wifi, requireWifiCredentials);
}

ValidationResult migrateConfig(DeviceConfig& config) {
    if (config.schemaVersion == kCurrentConfigSchemaVersion) {
        return validateConfig(config, false);
    }
    if (config.schemaVersion == 0 || config.schemaVersion > kCurrentConfigSchemaVersion) {
        return {ConfigError::UnsupportedSchemaVersion, "unsupported schema version"};
    }

    config.schemaVersion = kCurrentConfigSchemaVersion;
    if (config.maxJsonBytes == 0) {
        config.maxJsonBytes = kDefaultMaxJsonBytes;
    }
    if (config.deviceName.empty()) {
        config.deviceName = defaultConfig().deviceName;
    }
    return validateConfig(config, false);
}

} // namespace ewfm
