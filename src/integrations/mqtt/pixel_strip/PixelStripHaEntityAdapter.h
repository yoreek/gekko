#pragma once

#include "integrations/mqtt/HaEntityAdapter.h"

namespace ewfm {

// pixel_strip -> HA `light`, brightness only (mirrors AnalogOutputHaEntityAdapter). Color is
// deliberately not published here -- it belongs to whichever pixel_effect_* decorator currently
// controls the strip (see PixelEffectSolidHaEntityAdapter), matching the same PixelStrip-role
// exclusive-controller split the firmware/portal already use (docs/pixel-strip.md).
class PixelStripHaEntityAdapter final : public IHaEntityAdapter {
public:
    static const PixelStripHaEntityAdapter& instance();

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
