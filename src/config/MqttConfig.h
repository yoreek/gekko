#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ewfm {

constexpr size_t kMqttMaxHostLength = 128;
constexpr size_t kMqttMaxClientIdLength = 64;
constexpr size_t kMqttMaxUsernameLength = 64;
constexpr size_t kMqttMaxPasswordLength = 64;
constexpr size_t kMqttMaxDiscoveryPrefixLength = 64;
constexpr size_t kMqttMaxNodeIdLength = 48;
constexpr size_t kMqttMaxNodeNameLength = 64;
constexpr size_t kMqttMaxCaCertBytes = 8192;

struct MqttSettings {
    bool enabled{false};
    std::string host{};
    uint16_t port{1883};
    bool useTls{false};
    std::string clientId{};
    std::string username{};
    std::string password{};
    std::string haDiscoveryPrefix{"homeassistant"};
    std::string haNodeId{};
    std::string haNodeName{};
};

enum class MqttConfigError {
    None,
    HostRequired,
    HostTooLong,
    PortInvalid,
    ClientIdRequired,
    ClientIdTooLong,
    UsernameTooLong,
    PasswordTooLong,
    DiscoveryPrefixInvalid,
    NodeIdRequired,
    NodeIdTooLong,
    NodeIdInvalidCharacters,
    NodeNameTooLong,
    CertTooLarge,
    StorageError,
};

struct MqttConfigValidationResult {
    MqttConfigError error{MqttConfigError::None};
    const char* message{"ok"};

    bool ok() const {
        return error == MqttConfigError::None;
    }
};

MqttSettings defaultMqttSettings();
MqttConfigValidationResult validateMqttSettings(const MqttSettings& settings);
bool isValidHaNodeIdCharset(const std::string& value);

// Builds a valid default `haNodeId` from the device name and a MAC-derived suffix,
// mirroring WifiManager::setupApSsid()'s "<deviceName>-<macSuffix>" convention but
// sanitized to the [a-zA-Z0-9_-] charset the Home Assistant MQTT discovery topic
// scheme requires (deviceName itself has no such charset restriction).
std::string defaultHaNodeId(const std::string& deviceName, const std::string& macSuffix);

} // namespace ewfm
