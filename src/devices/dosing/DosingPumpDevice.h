#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/dosing/DosingPumpDeviceConfig.h"
#include "devices/dosing/DosingPumpSchedule.h"
#include "time/DateTime.h"

#include <ArduinoJson.h>

namespace ewfm {

enum class DosingRunType : uint8_t {
    Scheduled = 0,
    Manual = 1,
    Calibration = 2,
};

// Everything that must survive a reboot but changes too often for the versioned config blob
// (see the retained-state-vs-config rule): user intent (autoMode, skipNext), consumption state
// (container level, today's totals keyed by dayNumber), and the last logged dose. firedMask is
// per-dayNumber: which dose slots already ran (or were dropped/skipped) today - reloading it means
// a reboot never re-doses an already-handled slot.
struct DosingPumpRetainedStateV1 {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0};
    uint8_t autoMode{0};
    uint8_t lastDoseType{static_cast<uint8_t>(DosingRunType::Scheduled)};
    uint16_t skipNextMask{0};
    uint16_t firedMask{0};
    uint16_t lastDoseAmountCentiMl{0};
    uint32_t dayNumber{0}; // local unixtime / 86400 that firedMask/todayDosed belong to
    uint32_t todayDosedCentiMl{0};
    uint32_t containerCurrentCentiMl{0};
    uint32_t lastDoseEpoch{0};
};

// A peristaltic dosing pump channel: drives one required Switch-role dependency (the pump motor's
// relay/MOSFET switch) with precisely timed runs computed from the calibrated flow rate
// (runMs = amount / speed), evaluates its own per-day dose schedule (the minute-precision
// ScheduleDevice deliberately dropped this edge-triggered doser case), tracks container
// consumption, and optionally consumes a Condition-role low-level sensor (binary_sensor). The
// scheduled dose's fired bit is set when the run *starts*, so a reboot mid-run can never
// double-dose - the partial remainder is deliberately lost instead.
class DosingPumpDevice final : public DeviceRuntimeBase {
public:
    DosingPumpDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit DosingPumpDevice(const DosingPumpDeviceConfigV1& config);

    const DosingPumpDeviceConfigV1& config() const;
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
    void end(uint32_t now) override;

    // Read surface for the REST adapter / tests.
    bool autoMode() const;
    bool timeValid() const;
    bool running() const;
    DosingRunType runType() const;
    uint32_t runTargetCentiMl() const;
    uint32_t runDosedCentiMl() const; // live progress while running
    uint32_t runRemainingMs() const;
    uint32_t runTotalMs() const;
    uint32_t lastRunDosedCentiMl() const;
    uint32_t todayDosedCentiMl() const;
    uint32_t todayTargetCentiMlCached() const;
    bool nextDoseAt(uint32_t& outEpoch, uint16_t& outAmountCentiMl) const;
    uint32_t lastDoseEpoch() const;
    DosingRunType lastDoseType() const;
    uint16_t lastDoseAmountCentiMl() const;
    uint32_t containerCurrentCentiMl() const;
    bool containerEmpty() const;
    bool levelSensorPresent() const;
    uint16_t skipNextMask() const;
    // Whole days the container lasts at the schedule's average daily consumption; false when the
    // schedule is empty.
    bool daysLeft(uint32_t& outDays) const;

    // Portable core of the once-per-second schedule evaluation - takes an explicit wall-clock
    // DateTime, so native tests drive it directly (the tick calls it with DateTime::current()).
    void evaluateSchedule(const DateTime& nowWall, uint32_t nowUptimeMs);

    // Manual/calibration entry points (also exercised by handleCommand). startRun refuses while a
    // run is active ("Busy"), when not Ready, or when a scheduled dose is empty-blocked.
    bool startRun(DosingRunType type, uint16_t amountCentiMl, uint32_t now);
    bool stopRun(uint32_t now); // idempotent early stop

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
    bool sensorReportsEmpty() const;
    void setAutoMode(bool enabledMode);
    void finishRun(uint32_t now);
    void abortRunIfActive(uint32_t now);
    void tickReady(uint32_t now);
    void evaluateScheduleFromClock(uint32_t now);
    void refreshDerivedSchedule(uint32_t localDayNumber, uint16_t nowMinuteOfDay);
    void markRetainedDirty();

    DosingPumpDeviceConfigV1 config_{};
    DosingPumpRetainedStateV1 state_{};
    ISwitchOutputRuntime* pumpSwitch_{nullptr};
    const IStatusRuntime* levelSensor_{nullptr};
    bool levelSensorInvert_{false};

    // Active run bookkeeping - uptime()-relative, never persisted (a reboot mid-run drops the
    // remainder by design).
    bool runActive_{false};
    DosingRunType runType_{DosingRunType::Manual};
    uint16_t runTargetCentiMl_{0};
    uint32_t runStartMs_{0};
    uint32_t runEndMs_{0};
    uint32_t lastRunDosedCentiMl_{0};
    uint32_t lastProgressDirtyMs_{0};

    // Derived-from-clock cache refreshed by evaluateSchedule().
    bool timeValid_{false};
    uint32_t currentWallEpoch_{0};
    uint32_t todayTargetCentiMl_{0};
    bool nextDoseValid_{false};
    uint32_t nextDoseEpoch_{0};
    uint16_t nextDoseAmountCentiMl_{0};
    uint32_t lastScheduleEvalMs_{0};
    bool scheduleEvaluatedOnce_{false};

    bool retainedStateDirty_{false};
};

} // namespace ewfm
