#include "integrations/mqtt/HaDiscoveryBridge.h"

#include "debug/Debug.h"
#include "integrations/mqtt/HaDeviceSettings.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"
#include "integrations/mqtt/HaDiscoveryPayload.h"

namespace ewfm {

namespace {
std::vector<std::string> splitTopicSegments(const std::string& topic) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= topic.size()) {
        const size_t pos = topic.find('/', start);
        if (pos == std::string::npos) {
            parts.push_back(topic.substr(start));
            break;
        }
        parts.push_back(topic.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

bool parseDeviceIdSegment(const std::string& text, DeviceId& deviceId) {
    if (text.empty()) {
        return false;
    }
    uint32_t value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10U + static_cast<uint32_t>(c - '0');
    }
    deviceId = value;
    return true;
}
} // namespace

HaDiscoveryBridge::HaDiscoveryBridge(MqttManager* mqttManager, DeviceRegistry* deviceRegistry, DeviceEventDispatcher* dispatcher,
                                     DeviceScopedDataStore* haSettingsStore)
    : mqttManager_(mqttManager), deviceRegistry_(deviceRegistry), dispatcher_(dispatcher), haSettingsStore_(haSettingsStore) {}

void HaDiscoveryBridge::begin(const std::string& nodeId, const std::string& nodeName, const std::string& haDiscoveryPrefix) {
    nodeId_ = nodeId;
    nodeName_ = nodeName;
    haDiscoveryPrefix_ = haDiscoveryPrefix.empty() ? std::string{"homeassistant"} : haDiscoveryPrefix;
    begun_ = true;

    if (mqttManager_ == nullptr) {
        return;
    }
    mqttManager_->setWill(availabilityTopic(), "offline", true);
    mqttManager_->onConnect([this]() { onMqttConnected(); });
    mqttManager_->onMessage(
        [this](const std::string& topic, const uint8_t* payload, size_t length) { onMqttMessage(topic, payload, length); });
}

void HaDiscoveryBridge::attachDispatcher() {
    if (dispatcher_ != nullptr) {
        dispatcher_->registerSink(*this);
    }
}

void HaDiscoveryBridge::detachDispatcher() {
    if (dispatcher_ != nullptr) {
        dispatcher_->unregisterSink(*this);
    }
}

void HaDiscoveryBridge::onDeviceEvent(const DeviceEvent& event) {
    if (!begun_) {
        return;
    }

    switch (event.kind) {
    case DeviceEventKind::DeviceCreated:
    case DeviceEventKind::DeviceUpdated:
        refreshDevice(event.deviceId);
        return;
    case DeviceEventKind::DeviceDeleted:
        retractDiscovery(event.deviceId, event.typeId);
        return;
    case DeviceEventKind::StateChanged:
    case DeviceEventKind::RetainedStateChanged: {
        if (deviceRegistry_ == nullptr || haSettingsStore_ == nullptr) {
            return;
        }
        const IDeviceRuntime* runtime = deviceRegistry_->runtime(event.deviceId);
        if (runtime == nullptr || !runtime->enabled()) {
            return;
        }
        if (adapters_.find(runtime->typeId()) == nullptr) {
            return;
        }
        const HaDeviceSettingsRecord settings = loadHaDeviceSettings(*haSettingsStore_, event.deviceId);
        if (settings.enabled == 0U) {
            return;
        }
        adapters_.forEach(runtime->typeId(), [this, &event, runtime](const IHaEntityAdapter& adapter) {
            publishStateOnly(event.deviceId, *runtime, adapter);
        });
        return;
    }
    default:
        return;
    }
}

void HaDiscoveryBridge::tickFastLoop(uint32_t now) {
    (void)now;
}

void HaDiscoveryBridge::tick100ms(uint32_t now) {
    (void)now;
}

void HaDiscoveryBridge::tick1s(uint32_t now) {
    (void)now;
}

void HaDiscoveryBridge::refreshDevice(DeviceId deviceId) {
    if (!begun_ || deviceRegistry_ == nullptr || haSettingsStore_ == nullptr) {
        EWFM_MQTT_LOG_WARN("refreshDevice(%lu) aborted: bridge not begun or missing registry/settings store",
                           static_cast<unsigned long>(deviceId));
        return;
    }
    const IDeviceRuntime* runtime = deviceRegistry_->runtime(deviceId);
    if (runtime == nullptr) {
        EWFM_MQTT_LOG_WARN("refreshDevice(%lu) aborted: no runtime for that device id", static_cast<unsigned long>(deviceId));
        return;
    }
    if (adapters_.find(runtime->typeId()) == nullptr) {
        EWFM_MQTT_LOG_WARN("refreshDevice(%lu) aborted: no HA adapter registered for typeId=%u", static_cast<unsigned long>(deviceId),
                           static_cast<unsigned>(runtime->typeId()));
        return;
    }
    const HaDeviceSettingsRecord settings = loadHaDeviceSettings(*haSettingsStore_, deviceId);
    if (settings.enabled == 0U || !runtime->enabled()) {
        EWFM_MQTT_LOG_DEBUG("refreshDevice(%lu): device or ha settings disabled, retracting discovery",
                            static_cast<unsigned long>(deviceId));
        retractDiscovery(deviceId, runtime->typeId());
        return;
    }
    EWFM_MQTT_LOG_DEBUG("refreshDevice(%lu): publishing discovery+state", static_cast<unsigned long>(deviceId));
    adapters_.forEach(runtime->typeId(), [this, deviceId, runtime](const IHaEntityAdapter& adapter) {
        publishDiscoveryAndState(deviceId, *runtime, adapter);
    });
}

void HaDiscoveryBridge::onMqttConnected() {
    if (mqttManager_ == nullptr) {
        return;
    }
    mqttManager_->publish(availabilityTopic(), "online", true);
    mqttManager_->subscribe(nodeId_ + "/+/+/set");

    if (deviceRegistry_ == nullptr || haSettingsStore_ == nullptr) {
        return;
    }
    deviceRegistry_->forEachRuntime([this](const IDeviceRuntime& runtime) {
        if (adapters_.find(runtime.typeId()) == nullptr) {
            return;
        }
        const HaDeviceSettingsRecord settings = loadHaDeviceSettings(*haSettingsStore_, runtime.deviceId());
        if (settings.enabled == 0U || !runtime.enabled()) {
            retractDiscovery(runtime.deviceId(), runtime.typeId());
            return;
        }
        adapters_.forEach(runtime.typeId(), [this, &runtime](const IHaEntityAdapter& adapter) {
            publishDiscoveryAndState(runtime.deviceId(), runtime, adapter);
        });
    });
}

void HaDiscoveryBridge::onMqttMessage(const std::string& topic, const uint8_t* payload, size_t length) {
    if (deviceRegistry_ == nullptr) {
        return;
    }
    const std::vector<std::string> parts = splitTopicSegments(topic);
    if (parts.size() != 4 || parts[0] != nodeId_ || parts[3] != "set") {
        return;
    }
    DeviceId deviceId{0};
    if (!parseDeviceIdSegment(parts[2], deviceId)) {
        return;
    }
    IDeviceRuntime* runtime = deviceRegistry_->runtime(deviceId);
    if (runtime == nullptr || !runtime->enabled()) {
        return;
    }
    if (adapters_.find(runtime->typeId()) == nullptr) {
        return;
    }
    const std::string payloadStr(reinterpret_cast<const char*>(payload), length);
    const std::string& commandKey = parts[1];
    bool handled = false;
    adapters_.forEach(runtime->typeId(), [&](const IHaEntityAdapter& adapter) {
        if (!handled && adapter.applyCommand(*deviceRegistry_, *runtime, deviceId, commandKey, payloadStr, 0)) {
            handled = true;
        }
    });
    if (!handled) {
        EWFM_MQTT_LOG_WARN("unrecognized command payload for device %lu", static_cast<unsigned long>(deviceId));
    }
}

void HaDiscoveryBridge::publishDiscoveryAndState(DeviceId deviceId, const IDeviceRuntime& runtime, const IHaEntityAdapter& adapter) {
    if (mqttManager_ == nullptr || haSettingsStore_ == nullptr) {
        return;
    }
    const std::string uniqueId = uniqueIdFor(adapter, deviceId);
    const HaTopicBuilder topicFor = topicBuilderFor(deviceId, true);
    const HaDeviceSettingsRecord settings = loadHaDeviceSettings(*haSettingsStore_, deviceId);
    const std::string effectiveName = effectiveHaDeviceName(settings, runtime.name());

    // Sized for the largest entity type (thermostat/climate has 6 topic fields plus numeric
    // bounds) - GPIO switch/DS18B20 sensor payloads use a small fraction of this.
    StaticJsonDocument<1536> doc;
    JsonObject output = doc.to<JsonObject>();
    adapter.buildDiscoveryPayload(runtime, uniqueId, effectiveName, topicFor, output);
    writeHaDiscoveryEnvelope(output, nodeId_, nodeName_, "~/status");

    const std::string discoveryTopic = discoveryTopicFor(adapter, uniqueId);
    size_t length = 0;
    const bool published = publishHaDiscoveryPayload(*mqttManager_, discoveryTopic, doc, length);
    if (published) {
        EWFM_MQTT_LOG_DEBUG("discovery publish ok: topic=%s payloadBytes=%u", discoveryTopic.c_str(), static_cast<unsigned>(length));
    } else {
        EWFM_MQTT_LOG_WARN("discovery publish failed: topic=%s payloadBytes=%u", discoveryTopic.c_str(), static_cast<unsigned>(length));
    }
    publishStateOnly(deviceId, runtime, adapter);
}

void HaDiscoveryBridge::publishStateOnly(DeviceId deviceId, const IDeviceRuntime& runtime, const IHaEntityAdapter& adapter) {
    if (mqttManager_ == nullptr) {
        return;
    }
    const HaTopicBuilder topicFor = topicBuilderFor(deviceId, false);
    const HaStatePublisher publish = [this](const std::string& topic, const std::string& payload) {
        mqttManager_->publish(topic, payload, true);
    };
    adapter.publishState(runtime, topicFor, publish);
}

void HaDiscoveryBridge::retractDiscovery(DeviceId deviceId, DeviceTypeId typeId) {
    if (mqttManager_ == nullptr) {
        return;
    }
    adapters_.forEach(typeId, [this, deviceId](const IHaEntityAdapter& adapter) {
        const std::string uniqueId = uniqueIdFor(adapter, deviceId);
        mqttManager_->publish(discoveryTopicFor(adapter, uniqueId), "", true);
    });
}

std::string HaDiscoveryBridge::uniqueIdFor(const IHaEntityAdapter& adapter, DeviceId deviceId) const {
    return nodeId_ + "_" + adapter.typeName() + "_" + std::to_string(deviceId);
}

std::string HaDiscoveryBridge::discoveryTopicFor(const IHaEntityAdapter& adapter, const std::string& uniqueId) const {
    return haDiscoveryPrefix_ + "/" + adapter.haComponent() + "/" + nodeId_ + "/" + uniqueId + "/config";
}

HaTopicBuilder HaDiscoveryBridge::topicBuilderFor(DeviceId deviceId, bool discoveryPayload) const {
    const std::string& nodeId = nodeId_;
    return [nodeId, deviceId, discoveryPayload](const char* channel, const char* suffix) {
        const std::string prefix = discoveryPayload ? std::string{"~/"} : nodeId + "/";
        return prefix + channel + "/" + std::to_string(deviceId) + "/" + suffix;
    };
}

std::string HaDiscoveryBridge::availabilityTopic() const {
    return nodeId_ + "/status";
}

} // namespace ewfm
