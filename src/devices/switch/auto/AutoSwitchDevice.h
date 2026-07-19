#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/auto/AutoSwitchDeviceConfig.h"

#include <ArduinoJson.h>
#include <array>

namespace ewfm {

// Uses the same generic DeviceRetainedDataStore::load/save mechanism that AbstractOutputDevice
// uses for physical output state, but stores AutoSwitch's separate 4-way mode.
// pausedUntilEpoch is a wall-clock (DateTime::unixtime()) anchor, only meaningful when
// mode == Paused: it exists purely so a reboot can recover how much pause time was left, since
// the in-memory countdown (AutoSwitchDevice::pausedUntilMs_) is uptime()-relative and always
// resets to 0 on boot. See AutoSwitchDevice::saveRetainedState()/loadRetainedState().
struct AutoSwitchRetainedStateV1 {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0};
    uint8_t mode{static_cast<uint8_t>(AutoSwitchMode::Auto)};
    uint32_t pausedUntilEpoch{0};
};

// The ReefDuino-inspired "ScheduledSwitch": wraps one required Switch dependency (the real switch it
// drives) and a bounded list of optional Condition-role dependencies (kMaxAutoSwitchConditions),
// each with a per-link invert flag, ANDed together to decide the Auto-mode output. A condition can
// be a Schedule, a plain switch, or another AutoSwitchDevice (it provides IStatusRuntime itself, so
// chaining works uniformly). Itself provides DeviceRole::Switch so it can be dropped on the
// dashboard as an ordinary switch tile or depended on by anything that accepts a switch (e.g.
// ThermostatDevice). Mode is a single flat AutoSwitchMode (Off/On/Auto/Paused, mirrors
// ReefDuino's ScheduledSwitchMode) -- Paused is not a separate overlay flag on top of Auto, it is
// just another mode value, only reachable from Auto and only exited back to Auto (there is no
// separate "resume": the same "auto" command that enters Auto from Off/On also exits Paused).
// Mode - including Paused - is persisted as retained state so a reboot mid-pause resumes paused
// with the correct remaining time, not silently back in Auto (see saveRetainedState()).
class AutoSwitchDevice final : public DeviceRuntimeBase, public ISwitchOutputRuntime, public IStatusRuntime {
public:
    using StateType = ISwitchOutputRuntime::StateType;

    AutoSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit AutoSwitchDevice(const AutoSwitchDeviceConfigV1& config);

    const AutoSwitchDeviceConfigV1& config() const;
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

    const ISwitchOutputRuntime* switchOutputRuntime() const override {
        return this;
    }
    const IStatusRuntime* statusRuntime() const override {
        return this;
    }
    // IStatusRuntime: lets another AutoSwitchDevice chain this one as a Condition-role dependency,
    // uniformly with a schedule or a plain switch.
    bool isActive() const override {
        return currentOutputState();
    }
    StateType currentOutputState() const override;
    bool requestOutputState(StateType state, uint32_t now) override;

    AutoSwitchMode mode() const;
    bool paused() const;
    uint32_t pausedUntilMs() const;
    bool conditionsSatisfied() const;

    // Portable, native-testable core of saveRetainedState()/loadRetainedState()'s reboot-recovery
    // math - no DateTime dependency, just plain uptime()/wall-clock-epoch arithmetic, so this is
    // what native tests exercise directly (mirrors ScheduleDevice::isActiveAt() vs isActive()).
    // Converts the live, uptime()-relative pausedUntilMs_ into a wall-clock anchor to persist.
    static uint32_t computePausedUntilEpoch(uint32_t nowUptimeMs, uint32_t pausedUntilMs, uint32_t nowWallEpoch);
    // Recovers a fresh uptime()-relative deadline from a persisted wall-clock anchor. Returns false
    // (leaving outPausedUntilMs untouched) if the anchor has already passed - the caller should not
    // resume paused in that case.
    static bool recoverPausedUntilMs(uint32_t pausedUntilEpoch, uint32_t nowWallEpoch, uint32_t nowUptimeMs, uint32_t& outPausedUntilMs);

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
    StateType desiredOutputState() const;
    void applyDesiredOutput(uint32_t now);
    void setMode(AutoSwitchMode mode);

    AutoSwitchDeviceConfigV1 config_{};
    AutoSwitchMode mode_{AutoSwitchMode::Auto};
    // Only meaningful while mode_ == Paused; the uptime deadline at which Paused reverts to Auto.
    uint32_t pausedUntilMs_{0U};
    // Set whenever setMode() newly enters Auto or Paused - forces the target off on the very next
    // tick and defers the schedule check to the tick after that.
    bool forceOffPending_{false};
    ISwitchOutputRuntime* targetSwitch_{nullptr};

    struct ConditionSource {
        const IStatusRuntime* source{nullptr};
        bool invert{false};
    };
    std::array<ConditionSource, kMaxAutoSwitchConditions> conditions_{};
    uint8_t conditionCount_{0};

    bool retainedStateDirty_{false};
};

} // namespace ewfm
