#include "config/MqttConfig.h"

namespace ewfm {

namespace {
bool isNodeIdChar(char c) {
    const bool isAlnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    return isAlnum || c == '_' || c == '-';
}

std::string sanitizeForNodeId(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        result.push_back(isNodeIdChar(c) ? c : '_');
    }
    return result;
}
} // namespace

MqttSettings defaultMqttSettings() {
    return MqttSettings{};
}

bool isValidHaNodeIdCharset(const std::string& value) {
    for (char c : value) {
        if (!isNodeIdChar(c)) {
            return false;
        }
    }
    return true;
}

std::string defaultHaNodeId(const std::string& deviceName, const std::string& macSuffix) {
    std::string base = sanitizeForNodeId(deviceName);
    if (base.empty()) {
        base = "esp32";
    }
    std::string suffix = macSuffix.empty() ? std::string{"setup"} : macSuffix;

    std::string candidate = base + "-" + suffix;
    if (candidate.size() > kMqttMaxNodeIdLength) {
        candidate = candidate.substr(0, kMqttMaxNodeIdLength);
    }
    return candidate;
}

MqttConfigValidationResult validateMqttSettings(const MqttSettings& settings) {
    if (settings.enabled && settings.host.empty()) {
        return {MqttConfigError::HostRequired, "mqtt host is required when enabled"};
    }
    if (settings.host.size() > kMqttMaxHostLength) {
        return {MqttConfigError::HostTooLong, "mqtt host is too long"};
    }
    if (settings.enabled && settings.port == 0) {
        return {MqttConfigError::PortInvalid, "mqtt port is invalid"};
    }
    if (settings.enabled && settings.clientId.empty()) {
        return {MqttConfigError::ClientIdRequired, "mqtt client id is required when enabled"};
    }
    if (settings.clientId.size() > kMqttMaxClientIdLength) {
        return {MqttConfigError::ClientIdTooLong, "mqtt client id is too long"};
    }
    if (settings.username.size() > kMqttMaxUsernameLength) {
        return {MqttConfigError::UsernameTooLong, "mqtt username is too long"};
    }
    if (settings.password.size() > kMqttMaxPasswordLength) {
        return {MqttConfigError::PasswordTooLong, "mqtt password is too long"};
    }
    if (settings.haDiscoveryPrefix.empty() || settings.haDiscoveryPrefix.size() > kMqttMaxDiscoveryPrefixLength) {
        return {MqttConfigError::DiscoveryPrefixInvalid, "mqtt ha discovery prefix is invalid"};
    }
    if (settings.haNodeId.empty()) {
        return {MqttConfigError::NodeIdRequired, "mqtt ha node id is required"};
    }
    if (settings.haNodeId.size() > kMqttMaxNodeIdLength) {
        return {MqttConfigError::NodeIdTooLong, "mqtt ha node id is too long"};
    }
    if (!isValidHaNodeIdCharset(settings.haNodeId)) {
        return {MqttConfigError::NodeIdInvalidCharacters, "mqtt ha node id has invalid characters"};
    }
    if (settings.haNodeName.size() > kMqttMaxNodeNameLength) {
        return {MqttConfigError::NodeNameTooLong, "mqtt ha node name is too long"};
    }
    return {};
}

} // namespace ewfm
