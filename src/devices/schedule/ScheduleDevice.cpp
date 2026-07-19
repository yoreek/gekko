#include "devices/schedule/ScheduleDevice.h"

#include "time/DateTime.h"

#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS ScheduleDevice

namespace {
// Guards against evaluating schedules against a bogus clock: a dead-battery RTC or a never-synced
// system clock commonly reads back as some power-on-reset/epoch-zero date, always older than
// this. Mirrors the reference ReefDuino AbstractScheduledSwitch::isValidEpoch() year check.
constexpr uint16_t kScheduleMinValidYear = 2020U;

// Mirrors SwitchOperatingScheduleStateV1::_isSuitableDayOfWeek(currentTime) - takes the DateTime
// directly, same as the reference, rather than pre-extracting weekday/minute into separate
// parameters.
bool isSuitableDayOfWeek(const ScheduleRuleV1& rule, const DateTime& currentTime) {
    const uint8_t weekdayIndex = static_cast<uint8_t>(currentTime.weekday() - 1U);
    return (rule.weekDays & (1U << weekdayIndex)) != 0U;
}

// Mirrors SwitchOperatingScheduleStateV1::_isSuitableTimeRange(currentTime), minute precision.
bool isSuitableTimeRange(const ScheduleRuleV1& rule, const DateTime& currentTime) {
    const uint16_t minuteOfDay = currentTime.minutesOfDay();
    if (rule.startMinuteOfDay == rule.endMinuteOfDay) {
        return true;
    }
    if (rule.startMinuteOfDay < rule.endMinuteOfDay) {
        return minuteOfDay >= rule.startMinuteOfDay && minuteOfDay < rule.endMinuteOfDay;
    }
    return minuteOfDay >= rule.startMinuteOfDay || minuteOfDay < rule.endMinuteOfDay;
}

uint16_t windowMinutes(const ScheduleRuleV1& rule) {
    if (rule.startMinuteOfDay == rule.endMinuteOfDay) {
        return kScheduleMinutesPerDay;
    }
    if (rule.startMinuteOfDay < rule.endMinuteOfDay) {
        return rule.endMinuteOfDay - rule.startMinuteOfDay;
    }
    return static_cast<uint16_t>((kScheduleMinutesPerDay - rule.startMinuteOfDay) + rule.endMinuteOfDay);
}

// Duty-cycle read (not the reference's edge-triggered pulse - see the ScheduleDevice class
// comment): splits the window into intervalsPerWindow equal slices and treats the first
// durationMinutes of each slice as "on".
bool isSuitableInterval(const ScheduleRuleV1& rule, const DateTime& currentTime) {
    if (rule.intervalsPerWindow == 0U) {
        return false;
    }
    const uint16_t intervalMinutes = static_cast<uint16_t>(windowMinutes(rule) / rule.intervalsPerWindow);
    if (intervalMinutes == 0U) {
        return false;
    }
    uint16_t minuteOfDay = currentTime.minutesOfDay();
    if (minuteOfDay < rule.startMinuteOfDay) {
        minuteOfDay = static_cast<uint16_t>(minuteOfDay + kScheduleMinutesPerDay);
    }
    const uint16_t positionInInterval = static_cast<uint16_t>((minuteOfDay - rule.startMinuteOfDay) % intervalMinutes);
    return positionInInterval < rule.durationMinutes;
}

// Mirrors SwitchOperatingScheduleStateV1::isAllowsTurnOn(currentTime), minus the doser-only
// lastStartedAt/min-interval edge-triggering (dropped - see ScheduleDevice class comment).
bool isRuleActive(const ScheduleRuleV1& rule, const DateTime& currentTime) {
    if (rule.enabled == 0U) {
        return false;
    }
    if (!isSuitableDayOfWeek(rule, currentTime) || !isSuitableTimeRange(rule, currentTime)) {
        return false;
    }
    if (static_cast<ScheduleRuleMode>(rule.mode) == ScheduleRuleMode::Interval) {
        return isSuitableInterval(rule, currentTime);
    }
    return true;
}
} // namespace

ScheduleDevice::ScheduleDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : ScheduleDevice([&configBlob]() {
          ScheduleDeviceConfigV1 config{};
          (void)decodeValidatedFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

ScheduleDevice::ScheduleDevice(const ScheduleDeviceConfigV1& config) : DeviceRuntimeBase((PState)&ScheduleDevice::Idle), config_(config) {}

const ScheduleDeviceConfigV1& ScheduleDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& ScheduleDevice::baseConfig() const {
    return config_;
}

bool ScheduleDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = scheduleDeviceConfigSize(config_);
    return encodeFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan ScheduleDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    ScheduleDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    return {};
}

bool ScheduleDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    ScheduleDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    // The rules just changed - a cached result computed under the old rules is no longer valid
    // even if the minute hasn't.
    cacheValid_ = false;
    return true;
}
bool ScheduleDevice::isActive() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const DateTime now = DateTime::current();
    if (now.year() < kScheduleMinValidYear) {
        // Clock isn't trustworthy right now - don't let a stale cached answer from before it went
        // bad (or from a previous boot's leftover state) survive.
        cacheValid_ = false;
        return false;
    }
    const uint32_t minuteKey = now.unixtime() / 60U;
    if (!cacheValid_ || minuteKey != cachedMinuteKey_) {
        cachedActive_ = isActiveAt(now);
        cachedMinuteKey_ = minuteKey;
        cacheValid_ = true;
    }
    return cachedActive_;
#else
    return false;
#endif
}

bool ScheduleDevice::timeValid() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return DateTime::current().year() >= kScheduleMinValidYear;
#else
    return false;
#endif
}

bool ScheduleDevice::isActiveAt(const DateTime& currentTime) const {
    for (uint8_t index = 0U; index < config_.ruleCount; ++index) {
        if (isRuleActive(config_.rules[index], currentTime)) {
            return true;
        }
    }
    return false;
}

DeviceTypeDescriptor ScheduleDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kScheduleDeviceTypeId;
    descriptor.name = "ScheduleDevice";
    descriptor.currentConfigVersion = kScheduleDeviceConfigVersion;
    descriptor.maxDependents = 8;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.providedRoles = ProvidedRoles::of({IScheduleRuntime::kProvidedRole, IStatusRuntime::kProvidedRole});
    descriptor.createRuntime = &ScheduleDevice::createRuntime;
    descriptor.validateConfig = &ScheduleDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> ScheduleDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new ScheduleDevice(record, configBlob));
}

DeviceValidationResult ScheduleDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "schedule config exceeds supported size"};
    }
    ScheduleDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "schedule config is invalid"};
    }
    return config.validate();
}

SM_STATE(ScheduleDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(ScheduleDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }

    clearStartRequested();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(ScheduleDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(ScheduleDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(ScheduleDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (!disableRequested_ && config_.enabled != 0U) {
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(ScheduleDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
