#include "integrations/mqtt/system/SystemHaPublisher.h"

#include "debug/Debug.h"
#include "generated/Version.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"
#include "integrations/mqtt/HaDiscoveryPayload.h"
#include "portal/controllers/SystemRestartController.h"

#include <ArduinoJson.h>
#include <array>
#include <cstdio>

namespace ewfm {

namespace {
constexpr uint32_t kPublishIntervalMs = 30000;

enum class SystemHaEntityKind : uint8_t {
    Uptime,
    FreeHeap,
    HeapFragmentation,
    MinFreeHeap,
    LargestFreeBlock,
    WifiRssi,
    WifiSsid,
    WifiIp,
    FirmwareVersion,
    FirmwareBuildDate,
    Restart,
};

struct SystemHaEntityDescriptor {
    SystemHaEntityKind kind;
    const char* key;
    const char* component;
    const char* name;
    const char* deviceClass;
    const char* unit;
    const char* stateClass;
    const char* category;
    const char* icon;
};

constexpr std::array<SystemHaEntityDescriptor, 11> kSystemHaEntities{{
    {SystemHaEntityKind::Uptime, "uptime", ha::component::kSensor, "Uptime", "duration", "s", "measurement", "diagnostic",
     "mdi:timer-outline"},
    {SystemHaEntityKind::FreeHeap, "free_heap", ha::component::kSensor, "Free heap", nullptr, "bytes", "measurement", "diagnostic",
     "mdi:memory"},
    {SystemHaEntityKind::HeapFragmentation, "heap_fragmentation", ha::component::kSensor, "Heap fragmentation", nullptr, "%", "measurement",
     "diagnostic", "mdi:memory"},
    {SystemHaEntityKind::MinFreeHeap, "min_free_heap", ha::component::kSensor, "Min free heap", nullptr, "bytes", "measurement",
     "diagnostic", "mdi:memory"},
    {SystemHaEntityKind::LargestFreeBlock, "largest_free_block", ha::component::kSensor, "Largest free block", nullptr, "bytes",
     "measurement", "diagnostic", "mdi:memory"},
    {SystemHaEntityKind::WifiRssi, "wifi_rssi", ha::component::kSensor, "WiFi signal", "signal_strength", "dBm", "measurement",
     "diagnostic", nullptr},
    {SystemHaEntityKind::WifiSsid, "wifi_ssid", ha::component::kSensor, "WiFi SSID", nullptr, nullptr, nullptr, "diagnostic", "mdi:wifi"},
    {SystemHaEntityKind::WifiIp, "wifi_ip", ha::component::kSensor, "WiFi IP address", nullptr, nullptr, nullptr, "diagnostic",
     "mdi:ip-network"},
    {SystemHaEntityKind::FirmwareVersion, "firmware_version", ha::component::kSensor, "Firmware version", nullptr, nullptr, nullptr,
     "diagnostic", "mdi:chip"},
    {SystemHaEntityKind::FirmwareBuildDate, "firmware_build_date", ha::component::kSensor, "Firmware build date", "timestamp", nullptr,
     nullptr, "diagnostic", "mdi:calendar-clock"},
    {SystemHaEntityKind::Restart, "restart", ha::component::kButton, "Restart", "restart", nullptr, nullptr, "config", nullptr},
}};
} // namespace

SystemHaPublisher::SystemHaPublisher(MqttManager* mqttManager, WifiManager* wifiManager, const ISystemStats* systemStats,
                                     DeviceRegistry* deviceRegistry)
    : mqttManager_(mqttManager), wifiManager_(wifiManager), systemStats_(systemStats), deviceRegistry_(deviceRegistry) {}

void SystemHaPublisher::begin(const std::string& nodeId, const std::string& nodeName, const std::string& haDiscoveryPrefix) {
    nodeId_ = nodeId;
    nodeName_ = nodeName;
    haDiscoveryPrefix_ = haDiscoveryPrefix.empty() ? std::string{"homeassistant"} : haDiscoveryPrefix;
    begun_ = true;

    if (mqttManager_ == nullptr) {
        return;
    }
    mqttManager_->onConnect([this]() { onMqttConnected(); });
    mqttManager_->onMessage(
        [this](const std::string& topic, const uint8_t* payload, size_t length) { onMqttMessage(topic, payload, length); });
}

void SystemHaPublisher::tick(uint32_t now) {
    if (!begun_ || mqttManager_ == nullptr || !mqttManager_->connected()) {
        return;
    }
    if (everPublished_ && static_cast<uint32_t>(now - lastPublishAtMs_) < kPublishIntervalMs) {
        return;
    }
    lastPublishAtMs_ = now;
    everPublished_ = true;
    publishState(now);
}

void SystemHaPublisher::onMqttConnected() {
    if (mqttManager_ == nullptr) {
        return;
    }
    publishDiscovery();
    // Force the next tick() (moments away, same or next App::tick()) to publish fresh state
    // immediately rather than waiting out a stale kPublishIntervalMs window from before reconnect.
    everPublished_ = false;
}

void SystemHaPublisher::onMqttMessage(const std::string& topic, const uint8_t* payload, size_t length) {
    (void)payload;
    (void)length;
    if (topic != topicFor("restart", "set")) {
        return;
    }
    DeviceRegistryRestartPrecondition precondition(deviceRegistry_);
    const SystemRestartDecision decision = SystemRestartController::requestRestart(precondition);
    if (!decision.ok()) {
        EWFM_MQTT_LOG_WARN("mqtt-triggered restart rejected: %s", decision.validation.message);
        return;
    }
    EWFM_MQTT_LOG_INFO("system restart requested via Home Assistant");
    SystemRestartController::scheduleReboot();
}

void SystemHaPublisher::publishDiscovery() {
    if (mqttManager_ == nullptr) {
        return;
    }
    for (const auto& entity : kSystemHaEntities) {
        const std::string uniqueId = nodeId_ + "_system_" + entity.key;
        StaticJsonDocument<512> doc;
        JsonObject output = doc.to<JsonObject>();
        writeHaEntityIdentity(output, uniqueId, entity.name);
        if (entity.kind == SystemHaEntityKind::Restart) {
            output[ha::key::kCommandTopic] = topicFor(entity.key, ha::topic::kSet, true);
        } else {
            output[ha::key::kStateTopic] = topicFor(entity.key, ha::topic::kState, true);
        }
        if (entity.deviceClass != nullptr) {
            output[ha::key::kDeviceClass] = entity.deviceClass;
        }
        if (entity.unit != nullptr) {
            output[ha::key::kUnitOfMeasurement] = entity.unit;
        }
        if (entity.stateClass != nullptr) {
            output[ha::key::kStateClass] = entity.stateClass;
        }
        if (entity.category != nullptr) {
            output[ha::key::kEntityCategory] = entity.category;
        }
        if (entity.icon != nullptr) {
            output[ha::key::kIcon] = entity.icon;
        }
        writeHaDiscoveryEnvelope(output, nodeId_, nodeName_, "~/status");

        size_t length = 0;
        if (!publishHaDiscoveryPayload(*mqttManager_, discoveryTopicFor(entity.component, entity.key), doc, length)) {
            EWFM_MQTT_LOG_WARN("system discovery publish failed: key=%s payloadBytes=%u", entity.key, static_cast<unsigned>(length));
        }
    }
}

void SystemHaPublisher::publishState(uint32_t now) {
    if (mqttManager_ == nullptr) {
        return;
    }

    char buffer[16];
    for (const auto& entity : kSystemHaEntities) {
        const char* payload = buffer;
        switch (entity.kind) {
        case SystemHaEntityKind::Uptime:
            std::snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(now / 1000U));
            break;
        case SystemHaEntityKind::FreeHeap:
            if (systemStats_ == nullptr) {
                continue;
            }
            std::snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(systemStats_->freeHeapBytes()));
            break;
        case SystemHaEntityKind::HeapFragmentation:
            if (systemStats_ == nullptr) {
                continue;
            }
            std::snprintf(buffer, sizeof(buffer), "%u", static_cast<unsigned>(systemStats_->heapFragmentationPercent()));
            break;
        case SystemHaEntityKind::MinFreeHeap:
            if (systemStats_ == nullptr) {
                continue;
            }
            std::snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(systemStats_->minFreeHeapBytes()));
            break;
        case SystemHaEntityKind::LargestFreeBlock:
            if (systemStats_ == nullptr) {
                continue;
            }
            std::snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(systemStats_->largestFreeBlockBytes()));
            break;
        case SystemHaEntityKind::WifiRssi:
            if (wifiManager_ == nullptr) {
                continue;
            }
            std::snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(wifiManager_->rssi()));
            break;
        case SystemHaEntityKind::WifiSsid:
            if (wifiManager_ == nullptr) {
                continue;
            }
            mqttManager_->publish(topicFor(entity.key, ha::topic::kState), wifiManager_->ssid(), true);
            continue;
        case SystemHaEntityKind::WifiIp:
            if (wifiManager_ == nullptr) {
                continue;
            }
            mqttManager_->publish(topicFor(entity.key, ha::topic::kState), wifiManager_->stationIp(), true);
            continue;
        case SystemHaEntityKind::FirmwareVersion:
            payload = EWFM_FIRMWARE_VERSION;
            break;
        case SystemHaEntityKind::FirmwareBuildDate:
            payload = EWFM_FIRMWARE_BUILD_DATE;
            break;
        case SystemHaEntityKind::Restart:
            continue;
        }
        mqttManager_->publish(topicFor(entity.key, ha::topic::kState), payload, true);
    }
}

std::string SystemHaPublisher::topicFor(const char* key, const char* suffix, bool discoveryPayload) const {
    const std::string prefix = discoveryPayload ? std::string{"~/system/"} : nodeId_ + "/system/";
    return prefix + key + "/" + suffix;
}

std::string SystemHaPublisher::discoveryTopicFor(const char* component, const char* key) const {
    return haDiscoveryPrefix_ + "/" + component + "/" + nodeId_ + "/" + nodeId_ + "_system_" + key + "/config";
}

std::string SystemHaPublisher::availabilityTopic() const {
    return nodeId_ + "/status";
}

} // namespace ewfm
