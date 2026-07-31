#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/pixel/effects/PixelEffectSolidDeviceConfig.h"

namespace ewfm {

// Retained across reboot the same way AbstractOutputDevice's OutputDeviceRetainedStateRecord
// persists a live output value -- separate from the versioned config blob, so a SetOutput color
// change never bumps configRevision.
struct PixelEffectSolidRetainedStateV1 {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0};
    PixelColor color{};
    // Explicit on/off gate, independent of color -- see PixelEffectSolidDevice::applyColorIfNeeded().
    // Not derived from "color == black" (that comparison is what HA's on/off toggle used to be
    // computed from, and it caused picking black in the color picker to silently flip the toggle).
    bool on{true};
};

// The minimum viable pixel effect: fills the whole target strip with one static color. Depends on
// exactly one PixelStrip-role device and holds it exclusively (see descriptor() ->
// exclusiveDependencyRoles), the same "only one controlling dependent per target" rule
// FadeAnalogOutputDevice relies on over analog_output. Provides no role back -- it is a terminal
// decorator in v1, unlike Fade/Scheduled which re-provide AnalogOutput to support chaining.
class PixelEffectSolidDevice final : public DeviceRuntimeBase {
public:
    PixelEffectSolidDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit PixelEffectSolidDevice(const PixelEffectSolidDeviceConfigV1& config);

    const PixelEffectSolidDeviceConfigV1& config() const;
    void setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) override;
    void setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;
    bool handleCommand(const DeviceCommand& command) override;
    bool retainedStateDirty() const override;
    void clearRetainedStateDirty() override;
    DeviceValidationResult saveRetainedState(DeviceRetainedDataStore& store) const override;
    DeviceValidationResult loadRetainedState(DeviceRetainedDataStore& store) override;
    // Live, currently-shown color -- runtime state, not config. See PixelEffectSolidRetainedStateV1.
    PixelColor liveColor() const;
    // Explicit on/off gate -- runtime state, not config, not derived from liveColor(). See
    // PixelEffectSolidRetainedStateV1 and applyColorIfNeeded().
    bool liveOn() const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Reconfiguring();
    State Ready();
    State DependencyBlocked();
    State Disabled();
    State Deleting();

    void refreshCapabilityCache();
    bool dependenciesAvailable() const;
    bool dependencyBlocked() const;
    bool dependencyIsDisabled() const;
    void applyColorIfNeeded(uint32_t now);
    bool parseSetOutputColor(const DeviceCommand& command, PixelColor& color) const;
    bool parseSetOutputOn(const DeviceCommand& command, bool& on) const;

    PixelEffectSolidDeviceConfigV1 config_{};
    IPixelStripRuntime* targetStrip_{nullptr};
    PixelColor lastAppliedColor_{};
    bool colorApplied_{false};

    PixelColor liveColor_{};
    PixelColor retainedColor_{};
    bool retainedColorAvailable_{false};
    bool liveOn_{true};
    bool retainedOn_{true};
    bool retainedOnAvailable_{false};
    bool retainedStateDirty_{false};
};

} // namespace ewfm
