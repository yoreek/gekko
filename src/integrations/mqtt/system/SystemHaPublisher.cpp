#include "integrations/mqtt/system/SystemHaPublisher.h"

#include "debug/Debug.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"
#include "portal/controllers/SystemRestartController.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <functional>
#include <vector>

namespace ewfm {

namespace {
constexpr uint32_t kPublishIntervalMs = 30000;
}

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

namespace {
void publishOneDiscovery(MqttManager& mqttManager, const std::string& discoveryTopic, const std::string& nodeId,
                         const std::string& nodeName, const std::string& availabilityTopic, const char* uniqueId, const char* name,
                         const char* stateTopic, const char* commandTopic, const std::function<void(JsonObject)>& fillEntityFields) {
    DynamicJsonDocument doc(512);
    JsonObject output = doc.to<JsonObject>();
    output["unique_id"] = uniqueId;
    output["object_id"] = uniqueId;
    output["name"] = name;
    if (stateTopic != nullptr) {
        output["state_topic"] = stateTopic;
    }
    if (commandTopic != nullptr) {
        output["command_topic"] = commandTopic;
    }
    fillEntityFields(output);
    writeHaDiscoveryEnvelope(output, nodeId, nodeName, availabilityTopic);

    const size_t payloadSize = measureJson(doc) + 1U;
    std::vector<char> buffer(payloadSize);
    const size_t length = serializeJson(doc, buffer.data(), buffer.size());
    mqttManager.publish(discoveryTopic, std::string(buffer.data(), length), true);
}
} // namespace

void SystemHaPublisher::publishDiscovery() {
    if (mqttManager_ == nullptr) {
        return;
    }
    const std::string availability = availabilityTopic();

    publishOneDiscovery(*mqttManager_, discoveryTopicFor("sensor", "uptime"), nodeId_, nodeName_, availability,
                        (nodeId_ + "_system_uptime").c_str(), "Uptime", topicFor("uptime", "state").c_str(), nullptr, [](JsonObject o) {
                            o["device_class"] = "duration";
                            o["unit_of_measurement"] = "s";
                            o["state_class"] = "measurement";
                            o["entity_category"] = "diagnostic";
                            o["icon"] = "mdi:timer-outline";
                        });

    publishOneDiscovery(*mqttManager_, discoveryTopicFor("sensor", "free_heap"), nodeId_, nodeName_, availability,
                        (nodeId_ + "_system_free_heap").c_str(), "Free heap", topicFor("free_heap", "state").c_str(), nullptr,
                        [](JsonObject o) {
                            o["unit_of_measurement"] = "bytes";
                            o["state_class"] = "measurement";
                            o["entity_category"] = "diagnostic";
                            o["icon"] = "mdi:memory";
                        });

    publishOneDiscovery(*mqttManager_, discoveryTopicFor("sensor", "heap_fragmentation"), nodeId_, nodeName_, availability,
                        (nodeId_ + "_system_heap_fragmentation").c_str(), "Heap fragmentation",
                        topicFor("heap_fragmentation", "state").c_str(), nullptr, [](JsonObject o) {
                            o["unit_of_measurement"] = "%";
                            o["state_class"] = "measurement";
                            o["entity_category"] = "diagnostic";
                            o["icon"] = "mdi:memory";
                        });

    publishOneDiscovery(*mqttManager_, discoveryTopicFor("sensor", "wifi_rssi"), nodeId_, nodeName_, availability,
                        (nodeId_ + "_system_wifi_rssi").c_str(), "WiFi signal", topicFor("wifi_rssi", "state").c_str(), nullptr,
                        [](JsonObject o) {
                            o["device_class"] = "signal_strength";
                            o["unit_of_measurement"] = "dBm";
                            o["state_class"] = "measurement";
                            o["entity_category"] = "diagnostic";
                        });

    publishOneDiscovery(*mqttManager_, discoveryTopicFor("sensor", "wifi_ssid"), nodeId_, nodeName_, availability,
                        (nodeId_ + "_system_wifi_ssid").c_str(), "WiFi SSID", topicFor("wifi_ssid", "state").c_str(), nullptr,
                        [](JsonObject o) {
                            o["entity_category"] = "diagnostic";
                            o["icon"] = "mdi:wifi";
                        });

    publishOneDiscovery(*mqttManager_, discoveryTopicFor("sensor", "wifi_ip"), nodeId_, nodeName_, availability,
                        (nodeId_ + "_system_wifi_ip").c_str(), "WiFi IP address", topicFor("wifi_ip", "state").c_str(), nullptr,
                        [](JsonObject o) {
                            o["entity_category"] = "diagnostic";
                            o["icon"] = "mdi:ip-network";
                        });

    publishOneDiscovery(*mqttManager_, discoveryTopicFor("button", "restart"), nodeId_, nodeName_, availability,
                        (nodeId_ + "_system_restart").c_str(), "Restart", nullptr, topicFor("restart", "set").c_str(), [](JsonObject o) {
                            o["device_class"] = "restart";
                            o["entity_category"] = "config";
                        });
}

void SystemHaPublisher::publishState(uint32_t now) {
    if (mqttManager_ == nullptr) {
        return;
    }

    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(now / 1000U));
    mqttManager_->publish(topicFor("uptime", "state"), buffer, true);

    if (systemStats_ != nullptr) {
        std::snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(systemStats_->freeHeapBytes()));
        mqttManager_->publish(topicFor("free_heap", "state"), buffer, true);

        std::snprintf(buffer, sizeof(buffer), "%u", static_cast<unsigned>(systemStats_->heapFragmentationPercent()));
        mqttManager_->publish(topicFor("heap_fragmentation", "state"), buffer, true);
    }

    if (wifiManager_ != nullptr) {
        std::snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(wifiManager_->rssi()));
        mqttManager_->publish(topicFor("wifi_rssi", "state"), buffer, true);
        mqttManager_->publish(topicFor("wifi_ssid", "state"), wifiManager_->ssid(), true);
        mqttManager_->publish(topicFor("wifi_ip", "state"), wifiManager_->stationIp(), true);
    }
}

std::string SystemHaPublisher::topicFor(const char* key, const char* suffix) const {
    return nodeId_ + "/system/" + key + "/" + suffix;
}

std::string SystemHaPublisher::discoveryTopicFor(const char* component, const char* key) const {
    return haDiscoveryPrefix_ + "/" + component + "/" + nodeId_ + "/" + nodeId_ + "_system_" + key + "/config";
}

std::string SystemHaPublisher::availabilityTopic() const {
    return nodeId_ + "/status";
}

} // namespace ewfm
