#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ewfm {

constexpr uint32_t kCurrentConfigSchemaVersion = 1;
constexpr size_t kMaxSsidLength = 32;
constexpr size_t kMaxPasswordLength = 64;
constexpr size_t kMaxDeviceNameLength = 32;
constexpr size_t kDefaultMaxJsonBytes = 2048;
constexpr uint32_t kProvisioningSessionTimeoutMs = 3UL * 60UL * 1000UL;
constexpr size_t kMaxProvisioningSecretLength = 64;
constexpr size_t kMaxJsonSizeBytes = 8192;
constexpr size_t kMaxNtpServerLength = 64;
constexpr size_t kMaxTimezoneIdLength = 32;
constexpr uint32_t kDefaultNtpSyncIntervalSeconds = 3600;
constexpr uint32_t kMinNtpSyncIntervalSeconds = 60;
constexpr uint32_t kMaxNtpSyncIntervalSeconds = 24UL * 3600UL;

struct WiFiCredentials {
    std::string ssid;
    std::string password;

    bool hasCredentials() const {
        return !ssid.empty();
    }
};

struct ProvisioningConfig {
    bool httpPortalEnabled{true};
    bool mobileProvisioningEnabled{true};
    bool resetProvisionedOnStart{false};
    std::string proofOfPossession{"abcd1234"};
    bool setupApPasswordEnabled{false};
    std::string setupApPassword{};
    uint32_t sessionTimeoutMs{kProvisioningSessionTimeoutMs};
};

struct WifiRuntimeConfig {
    uint8_t maxConnectRetries{5};
    uint32_t connectTimeoutMs{15000};
    uint32_t retryDelayMs{3000};
};

struct FirmwareUpdateConfig {
    bool webOtaEnabled{false};
    size_t maxMetadataBytes{1024};
};

// NTP sync + timezone selection. Core infrastructure (like WiFi), not an optional integration -
// lives directly on DeviceConfig rather than a separate optional-feature config store.
struct TimeConfig {
    bool enabled{true};
    std::string ntpServer{"pool.ntp.org"};
    std::string timezoneId{"Etc/GMT"}; // fixed UTC+0, present in TimeZoneTable ("Greenwich Mean Time")
    uint32_t syncIntervalSeconds{kDefaultNtpSyncIntervalSeconds};
};

struct DeviceConfig {
    uint32_t schemaVersion{kCurrentConfigSchemaVersion};
    std::string deviceName{"esp32-wifi-manager"};
    WiFiCredentials wifi{};
    ProvisioningConfig provisioning{};
    WifiRuntimeConfig wifiRuntime{};
    FirmwareUpdateConfig firmwareUpdate{};
    TimeConfig time{};
    size_t maxJsonBytes{kDefaultMaxJsonBytes};
};

enum class ConfigError {
    None,
    UnsupportedSchemaVersion,
    EmptySsid,
    SsidTooLong,
    PasswordTooLong,
    DeviceNameEmpty,
    DeviceNameTooLong,
    JsonTooLarge,
    InvalidJson,
    StorageError,
    NtpServerTooLong,
    TimezoneInvalid,
    SyncIntervalInvalid,
};

struct ValidationResult {
    ConfigError error{ConfigError::None};
    const char* message{"ok"};

    bool ok() const {
        return error == ConfigError::None;
    }
};

DeviceConfig defaultConfig();
ValidationResult validateConfig(const DeviceConfig& config);
ValidationResult validateConfig(const DeviceConfig& config, bool requireWifiCredentials);
ValidationResult validateWifiCredentials(const WiFiCredentials& credentials, bool requireCredentials);
ValidationResult validateTimeConfig(const TimeConfig& time);
ValidationResult migrateConfig(DeviceConfig& config);

} // namespace ewfm
