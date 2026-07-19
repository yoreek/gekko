#pragma once

#include "core/StateMachine.h"
#include "devices/core/DeviceRuntimeBase.h"
#include "devices/schedule/ScheduleDeviceConfig.h"
#include "time/DateTime.h"

#include <ArduinoJson.h>

namespace ewfm {

// Holds up to kMaxScheduleRules time-of-day/weekday rules and reports whether any of them is
// currently active. Purely a function of (config, current local time) - no persisted runtime
// state, no dependencies. isActive()/timeValid() read DateTime::current() directly (Arduino-only,
// see DateTimeTimezone.cpp); isActiveAt() is the portable, native-testable rule-evaluation core
// that isActive() delegates to once it has a real DateTime to evaluate against.
//
// isActive()'s result only ever changes at a whole-minute boundary (every field a rule looks at -
// weekday(), minutesOfDay() - is minute-granular), so it caches the last computed result keyed by
// (weekday, minuteOfDay) rather than re-walking the rule list on every call. This matters because
// several AutoSwitchDevice instances can share one Schedule and each calls isActive() on its own
// 1s tick, so multiple calls per second against the same minute are the common case, not an edge
// case.
class ScheduleDevice final : public DeviceRuntimeBase, public IScheduleRuntime, public IStatusRuntime {
public:
    ScheduleDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    explicit ScheduleDevice(const ScheduleDeviceConfigV1& config);

    const ScheduleDeviceConfigV1& config() const;
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

    const IScheduleRuntime* scheduleRuntime() const override {
        return this;
    }
    const IStatusRuntime* statusRuntime() const override {
        return this;
    }

    // IScheduleRuntime, IStatusRuntime -- both declare an identical `bool isActive() const`, so
    // this single override satisfies both interfaces at once.
    bool isActive() const override;
    bool timeValid() const override;

    // Pure rule evaluation against an explicit DateTime - no wall-clock dependency, so this is
    // what native tests exercise directly.
    bool isActiveAt(const DateTime& currentTime) const;

    static DeviceTypeDescriptor descriptor();
    static std::unique_ptr<IDeviceRuntime> createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    static DeviceValidationResult validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);

private:
    const DeviceBaseConfigV1& baseConfig() const override;

    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State Disabled();
    State Deleting();

    ScheduleDeviceConfigV1 config_{};
    // now.unixtime() / 60 - a monotonically increasing minute counter (never repeats, unlike
    // minutesOfDay() which resets every midnight), so a single equality check is enough to know
    // whether a new minute has started.
    mutable bool cacheValid_{false};
    mutable uint32_t cachedMinuteKey_{0U};
    mutable bool cachedActive_{false};
};

} // namespace ewfm
