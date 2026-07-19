#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace ewfm {

class DeviceRegistry;

// Builds an MQTT topic for one of this entity's "channels" (e.g. "switch", "climate_mode") and a
// suffix ("state"/"set"/"config"). Channels beyond haComponent() let a single entity own several
// independent state/command topics (e.g. thermostat mode vs. setpoint) without HaDiscoveryBridge
// having to know how many topics any given device type needs.
using HaTopicBuilder = std::function<std::string(const char* channel, const char* suffix)>;
// Publishes one retained MQTT message for a state topic built via HaTopicBuilder.
using HaStatePublisher = std::function<void(const std::string& topic, const std::string& payload)>;

// The extensibility seam for Home Assistant MQTT discovery: HaDiscoveryBridge (the generic
// consumer) only ever talks to this interface and never hardcodes knowledge of a specific
// device type. Device-family behavior lives in focused adapters; adding a future device type means
// registering one more adapter instance, not touching the bridge.
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
    // payload_on/off, etc.) into `output`, requesting topics via `topicFor`. HaDiscoveryBridge fills
    // in the fields shared by every entity (device/origin/availability_topic/has_entity_name)
    // afterwards - this must not touch those.
    virtual void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                                       const HaTopicBuilder& topicFor, JsonObject output) const = 0;

    // Publishes every retained state message this entity currently has (one call to `publish` per
    // topic) - one for a simple switch/sensor, several for a multi-topic entity like a thermostat.
    // No-op if no meaningful state can be published right now.
    virtual void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const = 0;

    // Applies an incoming command payload (already routed to this device by topic) by mutating the
    // device through `registry` however this device type requires - a lightweight DeviceCommand for
    // simple runtime state (e.g. GPIO switch ON/OFF), or a full validated config replace for
    // persisted settings (e.g. thermostat mode/setpoint). `commandKey` is the topic channel segment
    // (see HaTopicBuilder), letting a single adapter distinguish between several command topics.
    // Returns false (and applies nothing) if the payload/channel isn't recognized.
    virtual bool applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId, const std::string& commandKey,
                              const std::string& payload, uint32_t now) const;
};

class HaEntityAdapterRegistry {
public:
    // Rejects an exact duplicate (same typeId and typeName); otherwise a device type may register
    // more than one adapter - one per HA entity it exposes (e.g. HTU21's temperature + humidity).
    bool registerAdapter(const IHaEntityAdapter& adapter);
    // Returns the first adapter registered for typeId, or nullptr. Useful where callers only need
    // to know whether a type has HA support at all.
    const IHaEntityAdapter* find(DeviceTypeId typeId) const;
    template <typename Callback> size_t forEach(DeviceTypeId typeId, Callback&& callback) const {
        size_t count = 0;
        for (const auto* adapter : adapters_) {
            if (adapter != nullptr && adapter->typeId() == typeId) {
                callback(*adapter);
                ++count;
            }
        }
        return count;
    }

    static HaEntityAdapterRegistry withDefaults();

private:
    std::vector<const IHaEntityAdapter*> adapters_{};
};

} // namespace ewfm
