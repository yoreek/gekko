#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

// pixel_effect_alert -> HA `binary_sensor`, reporting whether its ANDed condition list is currently
// satisfied (mirrors BinarySensorHaEntityAdapter / DosingPumpHaEntityAdapter's ContainerEmpty entity).
// Read-only: color/blinkIntervalMs are persisted config, not a live control surface (see
// docs/pixel-strip.md), so there is no applyCommand override.
class PixelEffectAlertHaEntityAdapter final : public IHaEntityAdapter {
public:
    static const PixelEffectAlertHaEntityAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const HaTopicBuilder& topicFor, JsonObject output) const override;
    void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const override;
};

} // namespace ewfm
