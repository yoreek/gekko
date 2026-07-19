#include "config/DeviceConfig.h"

#include "time/TimeZoneTable.h"

namespace ewfm {

DeviceConfig defaultConfig() {
    return DeviceConfig{};
}

ValidationResult validateConfig(const DeviceConfig& config) {
    return validateConfig(config, false);
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

ValidationResult validateTimeConfig(const TimeConfig& time) {
    if (time.ntpServer.empty() || time.ntpServer.size() > kMaxNtpServerLength) {
        return {ConfigError::NtpServerTooLong, "ntp server is required and must be reasonably short"};
    }
    if (time.timezoneId.size() > kMaxTimezoneIdLength || findTimeZoneEntry(time.timezoneId.c_str()) == nullptr) {
        return {ConfigError::TimezoneInvalid, "timezone id is not recognized"};
    }
    if (time.syncIntervalSeconds < kMinNtpSyncIntervalSeconds || time.syncIntervalSeconds > kMaxNtpSyncIntervalSeconds) {
        return {ConfigError::SyncIntervalInvalid, "sync interval is out of range"};
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
    if (config.maxJsonBytes == 0 || config.maxJsonBytes > kMaxJsonSizeBytes) {
        return {ConfigError::JsonTooLarge, "json limit is invalid"};
    }
    if (config.provisioning.proofOfPossession.size() > kMaxProvisioningSecretLength) {
        return {ConfigError::PasswordTooLong, "provisioning secret is too long"};
    }
    const ValidationResult timeResult = validateTimeConfig(config.time);
    if (!timeResult.ok()) {
        return timeResult;
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
