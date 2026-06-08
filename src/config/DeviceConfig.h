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

struct DeviceConfig {
    uint32_t schemaVersion{kCurrentConfigSchemaVersion};
    std::string deviceName{"esp32-wifi-manager"};
    WiFiCredentials wifi{};
    ProvisioningConfig provisioning{};
    WifiRuntimeConfig wifiRuntime{};
    FirmwareUpdateConfig firmwareUpdate{};
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
ValidationResult migrateConfig(DeviceConfig& config);

} // namespace ewfm
