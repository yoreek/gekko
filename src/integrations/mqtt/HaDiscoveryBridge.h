#pragma once

#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "integrations/common/DeviceEventDispatcher.h"
#include "integrations/mqtt/HaEntityAdapter.h"
#include "platform/MqttManager.h"

#include <cstdint>
#include <string>

namespace ewfm {

// Bridges DeviceRegistry changes to Home Assistant MQTT discovery. Knows nothing about specific
// device types beyond the IHaEntityAdapter interface - see HaEntityAdapter.h for the extensibility
// seam that keeps GPIO-switch-specific behavior out of this file.
class HaDiscoveryBridge final : public IDeviceEventSink {
public:
    HaDiscoveryBridge(MqttManager* mqttManager, DeviceRegistry* deviceRegistry, DeviceEventDispatcher* dispatcher,
                      DeviceScopedDataStore* haSettingsStore);

    void begin(const std::string& nodeId, const std::string& nodeName, const std::string& haDiscoveryPrefix);
    void attachDispatcher();
    void detachDispatcher();

    void onDeviceEvent(const DeviceEvent& event) override;
    void tickFastLoop(uint32_t now) override;
    void tick100ms(uint32_t now) override;
    void tick1s(uint32_t now) override;

    // Re-publishes (or retracts) discovery/state for a single device. Called by the portal
    // controller after a "setHaSettings" command changes that device's HA opt-in/name, since that
    // write bypasses DeviceRegistry and so never produces a DeviceEvent on its own.
    void refreshDevice(DeviceId deviceId);

private:
    void onMqttConnected();
    void onMqttMessage(const std::string& topic, const uint8_t* payload, size_t length);

    void publishDiscoveryAndState(DeviceId deviceId, const IDeviceRuntime& runtime, const IHaEntityAdapter& adapter);
    void publishStateOnly(DeviceId deviceId, const IDeviceRuntime& runtime, const IHaEntityAdapter& adapter);
    void retractDiscovery(DeviceId deviceId, DeviceTypeId typeId);

    std::string uniqueIdFor(const IHaEntityAdapter& adapter, DeviceId deviceId) const;
    std::string discoveryTopicFor(const IHaEntityAdapter& adapter, const std::string& uniqueId) const;
    std::string stateTopicFor(const IHaEntityAdapter& adapter, DeviceId deviceId) const;
    std::string commandTopicFor(const IHaEntityAdapter& adapter, DeviceId deviceId) const;
    std::string availabilityTopic() const;

    MqttManager* mqttManager_{nullptr};
    DeviceRegistry* deviceRegistry_{nullptr};
    DeviceEventDispatcher* dispatcher_{nullptr};
    DeviceScopedDataStore* haSettingsStore_{nullptr};
    HaEntityAdapterRegistry adapters_{HaEntityAdapterRegistry::withDefaults()};
    std::string nodeId_{};
    std::string nodeName_{};
    std::string haDiscoveryPrefix_{"homeassistant"};
    bool begun_{false};
};

} // namespace ewfm
