#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

// pixel_effect_solid -> HA `light`, RGB color only (mirrors PixelStripHaEntityAdapter's
// brightness-only split, see docs/pixel-strip.md). Brightness is deliberately not published here --
// it belongs to the pixel_strip this effect targets, matching the same PixelStrip-role exclusive-
// controller split the firmware/portal already use. The on/off channel has no independent memory of
// "the color before OFF" (the device model has none): OFF sends black, ON sends white, exactly as
// AnalogOutputHaEntityAdapter maps OFF/ON to 0%/100% for a device with no separate on/off state.
class PixelEffectSolidHaEntityAdapter final : public IHaEntityAdapter {
public:
    static const PixelEffectSolidHaEntityAdapter& instance();

    DeviceTypeId typeId() const override;
    const char* typeName() const override;
    const char* haComponent() const override;
    void buildDiscoveryPayload(const IDeviceRuntime& runtime, const std::string& uniqueId, const std::string& effectiveName,
                               const HaTopicBuilder& topicFor, JsonObject output) const override;
    void publishState(const IDeviceRuntime& runtime, const HaTopicBuilder& topicFor, const HaStatePublisher& publish) const override;
    bool applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId, const std::string& commandKey,
                      const std::string& payload, uint32_t now) const override;
};

} // namespace ewfm
