#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <string>
#include <vector>

namespace ewfm {

// The extensibility seam for Home Assistant MQTT discovery: HaDiscoveryBridge (the generic
// consumer) only ever talks to this interface and never hardcodes knowledge of a specific
// device type. All GPIO-switch-specific behavior lives in GpioSwitchHaEntityAdapter; adding a
// future device type means adding one more adapter, not touching the bridge.
class IHaEntityAdapter {
public:
    IHaEntityAdapter() = default;
    IHaEntityAdapter(const IHaEntityAdapter&) = delete;
    IHaEntityAdapter& operator=(const IHaEntityAdapter&) = delete;
    IHaEntityAdapter(IHaEntityAdapter&&) = delete;
    IHaEntityAdapter& operator=(IHaEntityAdapter&&) = delete;
    virtual ~IHaEntityAdapter() = default;

    virtual DeviceTypeId typeId() const = 0;
    // Used to build a self-documenting unique_id/topic segment (e.g. "gpio_switch"); not shown in the HA UI.
    virtual const char* typeName() const = 0;
    // The HA MQTT discovery component this device type maps to (e.g. "switch").
    virtual const char* haComponent() const = 0;

    // Fills entity-specific discovery fields (unique_id, object_id, name, state/command topics,
    // payload_on/off, etc.) into `output`. HaDiscoveryBridge fills in the fields shared by every
    // entity (device/origin/availability_topic/has_entity_name) afterwards - this must not touch those.
    virtual void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                                       const std::string& stateTopic, const std::string& commandTopic, JsonObject output) const = 0;

    // Builds the state payload for the entity's current runtime state (e.g. "ON"/"OFF"). Returns
    // false if no meaningful state can be published right now (bridge skips the publish).
    virtual bool buildStatePayload(const IDeviceRuntime& runtime, std::string& payload) const = 0;

    // Translates an incoming command payload (already routed to this device by topic) into a DeviceCommand.
    virtual bool parseCommand(const std::string& payload, DeviceId deviceId, DeviceCommand& command) const = 0;
};

class HaEntityAdapterRegistry {
public:
    bool registerAdapter(const IHaEntityAdapter& adapter);
    const IHaEntityAdapter* find(DeviceTypeId typeId) const;

    static HaEntityAdapterRegistry withDefaults();

private:
    std::vector<const IHaEntityAdapter*> adapters_{};
};

} // namespace ewfm
