#include "config/MqttConfigStore.h"

#include "debug/Debug.h"

namespace ewfm {

namespace {
constexpr const char* kNamespace = "mqtt";
constexpr const char* kEnabled = "enabled";
constexpr const char* kHost = "host";
constexpr const char* kPort = "port";
constexpr const char* kUseTls = "use_tls";
constexpr const char* kClientId = "client_id";
constexpr const char* kUsername = "username";
constexpr const char* kPassword = "password";
constexpr const char* kHaDiscoveryPrefix = "ha_prefix";
constexpr const char* kHaNodeId = "ha_node_id";
constexpr const char* kHaNodeName = "ha_node_name";
constexpr const char* kCaCert = "ca_cert";
} // namespace

bool MqttConfigStore::begin() {
    return storage_.begin(kNamespace, false);
}

MqttConfigValidationResult MqttConfigStore::load() {
    settings_ = defaultMqttSettings();

    storage_.getBool(kEnabled, settings_.enabled);
    storage_.getString(kHost, settings_.host);
    uint32_t port{settings_.port};
    if (storage_.getUInt(kPort, port)) {
        settings_.port = static_cast<uint16_t>(port);
    }
    storage_.getBool(kUseTls, settings_.useTls);
    storage_.getString(kClientId, settings_.clientId);
    storage_.getString(kUsername, settings_.username);
    storage_.getString(kPassword, settings_.password);
    storage_.getString(kHaDiscoveryPrefix, settings_.haDiscoveryPrefix);
    storage_.getString(kHaNodeId, settings_.haNodeId);
    storage_.getString(kHaNodeName, settings_.haNodeName);

    return {};
}

MqttConfigValidationResult MqttConfigStore::save(const MqttSettings& settings) {
    const MqttConfigValidationResult validation = validateMqttSettings(settings);
    if (!validation.ok()) {
        return validation;
    }

    const bool saved = storage_.putBool(kEnabled, settings.enabled) && storage_.putString(kHost, settings.host) &&
                       storage_.putUInt(kPort, settings.port) && storage_.putBool(kUseTls, settings.useTls) &&
                       storage_.putString(kClientId, settings.clientId) && storage_.putString(kUsername, settings.username) &&
                       storage_.putString(kPassword, settings.password) &&
                       storage_.putString(kHaDiscoveryPrefix, settings.haDiscoveryPrefix) &&
                       storage_.putString(kHaNodeId, settings.haNodeId) && storage_.putString(kHaNodeName, settings.haNodeName);

    if (!saved) {
        EWFM_CONFIG_LOG_WARN("failed to save mqtt configuration");
        return {MqttConfigError::StorageError, "failed to save mqtt configuration"};
    }

    settings_ = settings;
    EWFM_CONFIG_LOG_INFO("mqtt configuration saved");
    return {};
}

bool MqttConfigStore::hasCaCert() const {
    return storage_.hasKey(kCaCert);
}

MqttConfigValidationResult MqttConfigStore::loadCaCert(std::vector<uint8_t>& pem) const {
    pem.clear();
    if (!storage_.hasKey(kCaCert)) {
        return {};
    }
    if (!storage_.getBlob(kCaCert, pem)) {
        return {MqttConfigError::StorageError, "failed to load ca certificate"};
    }
    return {};
}

MqttConfigValidationResult MqttConfigStore::saveCaCert(const uint8_t* data, size_t size) {
    if (size > kMqttMaxCaCertBytes) {
        return {MqttConfigError::CertTooLarge, "ca certificate exceeds supported size"};
    }
    if (!storage_.putBlob(kCaCert, data, size)) {
        EWFM_CONFIG_LOG_WARN("failed to save mqtt ca certificate");
        return {MqttConfigError::StorageError, "failed to save ca certificate"};
    }
    return {};
}

bool MqttConfigStore::clearCaCert() {
    return storage_.remove(kCaCert);
}

} // namespace ewfm
