#include "integrations/mqtt/pixel_strip/PixelEffectAlertHaEntityAdapter.h"

#include "devices/pixel/effects/PixelEffectAlertDevice.h"
#include "integrations/mqtt/HaDiscoveryConstants.h"
#include "integrations/mqtt/HaDiscoveryEnvelope.h"

namespace ewfm {

const PixelEffectAlertHaEntityAdapter& PixelEffectAlertHaEntityAdapter::instance() {
    static const PixelEffectAlertHaEntityAdapter adapter;
    return adapter;
}

DeviceTypeId PixelEffectAlertHaEntityAdapter::typeId() const {
    return PixelEffectAlertDevice::descriptor().typeId;
}

const char* PixelEffectAlertHaEntityAdapter::typeName() const {
    return "pixel_effect_alert";
}

const char* PixelEffectAlertHaEntityAdapter::haComponent() const {
    return ha::component::kBinarySensor;
}

void PixelEffectAlertHaEntityAdapter::buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId,
                                                            const std::string& effectiveName, const HaTopicBuilder& topicFor,
                                                            JsonObject output) const {
    (void)runtime;
    writeHaEntityIdentity(output, uniqueId, effectiveName);
    output[ha::key::kStateTopic] = topicFor(ha::component::kBinarySensor, ha::topic::kState);
    output[ha::key::kPayloadOn] = ha::payload::kOn;
    output[ha::key::kPayloadOff] = ha::payload::kOff;
    output[ha::key::kIcon] = "mdi:alarm-light-outline";
}

void PixelEffectAlertHaEntityAdapter::publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor,
                                                   const HaStatePublisher& publish) const {
    const auto& alert = static_cast<const PixelEffectAlertDevice&>(runtime);
    publish(topicFor(ha::component::kBinarySensor, ha::topic::kState), alert.conditionsSatisfied() ? ha::payload::kOn : ha::payload::kOff);
}

} // namespace ewfm
