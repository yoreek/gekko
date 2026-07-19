#include "integrations/mqtt/binary/BinarySensorHaEntityAdapter.h"

#include "devices/sensors/binary/BinarySensorDevice.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

namespace ewfm {

const BinarySensorHaEntityAdapter& BinarySensorHaEntityAdapter::instance() {
    static const BinarySensorHaEntityAdapter adapter;
    return adapter;
}

DeviceTypeId BinarySensorHaEntityAdapter::typeId() const {
    return BinarySensorDevice::descriptor().typeId;
}

const char* BinarySensorHaEntityAdapter::typeName() const {
    return "binary_sensor";
}

const char* BinarySensorHaEntityAdapter::haComponent() const {
    return ha::component::kBinarySensor;
}

void BinarySensorHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                        const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                        JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kStateTopic] = topicFor(ha::component::kBinarySensor, ha::topic::kState);
    output[ha::key::kPayloadOn] = ha::payload::kOn;
    output[ha::key::kPayloadOff] = ha::payload::kOff;
    // No device_class: the firmware config carries no semantic hint (door/moisture/...), so the
    // entity stays a generic contact; users can override the class per-entity in HA if desired.
    output[ha::key::kIcon] = "mdi:electric-switch";
}

void BinarySensorHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                               const HaStatePublisher& publish) const {
    // IStatusRuntime only exposes isActive(); hasReading() needs the concrete device. Safe: the
    // bridge routes by runtime->typeId(), same idiom as ThermostatHaEntityAdapter.
    const auto& sensor = static_cast<const BinarySensorDevice&>(runtime);
    if (!sensor.hasReading()) {
        return; // no debounced level seen yet - skip rather than publish a guess
    }
    publish(topicFor(ha::component::kBinarySensor, ha::topic::kState), sensor.isActive() ? ha::payload::kOn : ha::payload::kOff);
}

} // namespace ewfm
