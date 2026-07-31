#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/pixel/effects/PixelEffectAlertDeviceConfig.h"

#include <array>

namespace ewfm {

// Blinks the target strip between `color` and black at `blinkIntervalMs` while a bounded, ANDed
// list of Condition-role dependencies is satisfied; holds steady black otherwise. Copies
// AutoSwitchDevice's dual-dependency shape (one required PixelStrip target + an optional
// Condition-role AND list read from `deps`, not from the config blob) and its
// conditionsSatisfied() semantics: an empty condition list is never satisfied, the same safe
// default AutoSwitchDevice uses, so a misconfigured alert can't blink constantly by accident.
class PixelEffectAlertDevice final : public DeviceRuntimeBase {
public:
    PixelEffectAlertDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit PixelEffectAlertDevice(const PixelEffectAlertDeviceConfigV1& config);

    const PixelEffectAlertDeviceConfigV1& config() const;
    void setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) override;
    void setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    bool conditionsSatisfied() const;

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
    void updateBlink(uint32_t now);

    struct ConditionSource {
        const IStatusRuntime* source{nullptr};
        bool invert{false};
    };

    PixelEffectAlertDeviceConfigV1 config_{};
    IPixelStripRuntime* targetStrip_{nullptr};
    std::array<ConditionSource, kMaxPixelEffectAlertConditions> conditions_{};
    uint8_t conditionCount_{0};

    bool hasApplied_{false};
    bool appliedOn_{false};
    // Tracks conditionsSatisfied() as of the last tick, so an unsatisfied->satisfied transition is
    // detected and repaints immediately on its own on-phase instead of waiting out whatever is left
    // of a blink timer that was counting down for a completely different (black, unsatisfied) state.
    bool wasSatisfied_{false};
    uint32_t lastToggleAt_{0U};
};

} // namespace ewfm
