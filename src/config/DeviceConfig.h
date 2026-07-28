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

// Device-registry persistence debounce: how long a dirty config/layout can sit in RAM before it is
// flushed to flash. debounceMs is the quiet-period timer (fires this long after the *last* change);
// maxDelayMs is the hard cap measured from the *first* unflushed change, regardless of continued
// activity -- it is the real ceiling on flash write frequency under continuous churn.
constexpr uint32_t kDefaultPersistenceDebounceMs = 500;
constexpr uint32_t kMinPersistenceDebounceMs = 100;
constexpr uint32_t kMaxPersistenceDebounceMs = 10000;
constexpr uint32_t kDefaultPersistenceMaxDelayMs = 30000;
constexpr uint32_t kMinPersistenceMaxDelayMs = 1000;
constexpr uint32_t kMaxPersistenceMaxDelayMs = 300000;

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

// See DeviceRegistry::tick()/DeviceRegistryPersistenceCoordinator::shouldFlush() for how these
// two values gate the debounced flash write.
struct PersistenceConfig {
    uint32_t debounceMs{kDefaultPersistenceDebounceMs};
    uint32_t maxDelayMs{kDefaultPersistenceMaxDelayMs};
};

struct DeviceConfig {
    uint32_t schemaVersion{kCurrentConfigSchemaVersion};
    std::string deviceName{"gekko"};
    WiFiCredentials wifi{};
    ProvisioningConfig provisioning{};
    WifiRuntimeConfig wifiRuntime{};
    FirmwareUpdateConfig firmwareUpdate{};
    TimeConfig time{};
    PersistenceConfig persistence{};
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
    PersistenceDebounceInvalid,
    PersistenceMaxDelayInvalid,
    PersistenceDebounceExceedsMaxDelay,
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
ValidationResult validatePersistenceConfig(const PersistenceConfig& persistence);
ValidationResult migrateConfig(DeviceConfig& config);

} // namespace ewfm
