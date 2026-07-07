#pragma once

#include "core/SystemStats.h"
#include "platform/MqttManager.h"
#include "wifi/WifiManager.h"

#include <cstdint>
#include <string>

namespace ewfm {

class DeviceRegistry;

// Publishes firmware-level diagnostics (uptime, free heap, heap fragmentation, WiFi RSSI/SSID/IP)
// and a restart button to Home Assistant, under the same HA device as the per-DeviceRegistry-device
// entities HaDiscoveryBridge publishes. Deliberately NOT an IHaEntityAdapter/DeviceRegistry consumer -
// these entities don't correspond to any device in the registry, they describe the board itself.
class SystemHaPublisher final {
public:
    SystemHaPublisher(MqttManager* mqttManager, WifiManager* wifiManager, const ISystemStats* systemStats, DeviceRegistry* deviceRegistry);

    void begin(const std::string& nodeId, const std::string& nodeName, const std::string& haDiscoveryPrefix);

    // Safe to call every App::tick() - internally throttles the actual MQTT publish to once every
    // kPublishIntervalMs; only the cheap "is it due yet" check runs on every call.
    void tick(uint32_t now);

private:
    void onMqttConnected();
    void onMqttMessage(const std::string& topic, const uint8_t* payload, size_t length);
    void publishDiscovery();
    void publishState(uint32_t now);
    std::string topicFor(const char* key, const char* suffix) const;
    std::string discoveryTopicFor(const char* component, const char* key) const;
    std::string availabilityTopic() const;

    MqttManager* mqttManager_{nullptr};
    WifiManager* wifiManager_{nullptr};
    const ISystemStats* systemStats_{nullptr};
    DeviceRegistry* deviceRegistry_{nullptr};
    std::string nodeId_{};
    std::string nodeName_{};
    std::string haDiscoveryPrefix_{"homeassistant"};
    uint32_t lastPublishAtMs_{0};
    bool everPublished_{false};
    bool begun_{false};
};

} // namespace ewfm
