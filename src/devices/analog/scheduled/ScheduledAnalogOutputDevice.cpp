#include "devices/analog/scheduled/ScheduledAnalogOutputDevice.h"

#include "devices/core/ConfigCodec.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "time/DateTime.h"

#include <algorithm>
#include <cstring>

namespace ewfm {

namespace {
constexpr uint16_t kMinimumValidYear = 2020U;
}

ScheduledAnalogOutputDevice::ScheduledAnalogOutputDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : ScheduledAnalogOutputDevice([&configBlob]() {
          ScheduledAnalogOutputDeviceConfigV2 config{};
          (void)decodeScheduledAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

ScheduledAnalogOutputDevice::ScheduledAnalogOutputDevice(const ScheduledAnalogOutputDeviceConfigV2& config) : config_(config) {}

const ScheduledAnalogOutputDeviceConfigV2& ScheduledAnalogOutputDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& ScheduledAnalogOutputDevice::baseConfig() const {
    return config_;
}

AnalogOutputMode ScheduledAnalogOutputDevice::analogOutputMode() const {
    return mode_;
}

bool ScheduledAnalogOutputDevice::requestAnalogOutputMode(const AnalogOutputMode mode, const uint32_t now) {
    if (status() != DeviceStatus::Ready ||
        (mode != AnalogOutputMode::Off && mode != AnalogOutputMode::Manual && mode != AnalogOutputMode::Scheduled)) {
        return false;
    }
    if (mode == AnalogOutputMode::Off) {
        setMode(mode);
        return currentOutputState() == 0U || forwardOutputState(0U, now);
    }
    if (mode == AnalogOutputMode::Manual) {
        manualState_ = currentOutputState();
        retainedStateDirty_ = true;
        setMode(mode);
        return forwardOutputState(manualState_, now);
    }
    setMode(mode);
    updateScheduledState(now, true);
    return true;
}

uint16_t ScheduledAnalogOutputDevice::requestedAnalogOutputState() const {
    if (mode_ == AnalogOutputMode::Off) {
        return 0U;
    }
    return mode_ == AnalogOutputMode::Manual ? manualState_ : scheduledState_;
}

bool ScheduledAnalogOutputDevice::analogOutputTimeValid() const {
    return timeValid_;
}

bool ScheduledAnalogOutputDevice::requestOutputState(const uint16_t state, const uint32_t now) {
    if (state > kAnalogOutputLevelMax || status() != DeviceStatus::Ready) {
        return false;
    }
    manualState_ = state;
    setMode(AnalogOutputMode::Manual);
    retainedStateDirty_ = true;
    return forwardOutputState(state, now);
}

bool ScheduledAnalogOutputDevice::handleCommand(const DeviceCommand& command) {
    if (command.type == DeviceCommandType::Custom && command.payload.equals("off")) {
        return requestAnalogOutputMode(AnalogOutputMode::Off, uptime());
    }
    if (command.type == DeviceCommandType::Custom && command.payload.equals("manual")) {
        return requestAnalogOutputMode(AnalogOutputMode::Manual, uptime());
    }
    if (command.type == DeviceCommandType::Custom && command.payload.equals("scheduled")) {
        return requestAnalogOutputMode(AnalogOutputMode::Scheduled, uptime());
    }
    return AnalogOutputDecoratorDeviceBase::handleCommand(command);
}

bool ScheduledAnalogOutputDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = scheduledAnalogOutputDeviceConfigSize(config_);
    return encodeFixedConfigBlob(ScheduledAnalogOutputDeviceConfigV2::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan ScheduledAnalogOutputDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    ScheduledAnalogOutputDeviceConfigV2 next{};
    (void)decodeScheduledAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), next);
    return {};
}

bool ScheduledAnalogOutputDevice::applyConfig(const DeviceConfigBlob& configBlob, const uint32_t now) {
    ScheduledAnalogOutputDeviceConfigV2 next{};
    if (!decodeScheduledAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return false;
    }
    config_ = next;
    cacheValid_ = false;
    if (mode_ == AnalogOutputMode::Scheduled && status() == DeviceStatus::Ready) {
        updateScheduledState(now, true);
    }
    return true;
}

bool ScheduledAnalogOutputDevice::retainedStateDirty() const {
    return retainedStateDirty_;
}

void ScheduledAnalogOutputDevice::clearRetainedStateDirty() {
    retainedStateDirty_ = false;
}

DeviceValidationResult ScheduledAnalogOutputDevice::saveRetainedState(DeviceRetainedDataStore& store) const {
    ScheduledAnalogOutputRetainedStateV1 state{};
    state.deviceId = deviceId();
    state.mode = static_cast<uint8_t>(mode_);
    state.manualState = manualState_;
    return store.save(state);
}

DeviceValidationResult ScheduledAnalogOutputDevice::loadRetainedState(DeviceRetainedDataStore& store) {
    ScheduledAnalogOutputRetainedStateV1 state{};
    const DeviceValidationResult result = store.load(deviceId(), state);
    if (!result.ok()) {
        return result;
    }
    if (state.mode <= static_cast<uint8_t>(AnalogOutputMode::Off) && state.manualState <= kAnalogOutputLevelMax) {
        mode_ = static_cast<AnalogOutputMode>(state.mode);
        manualState_ = state.manualState;
    }
    return {};
}

uint16_t ScheduledAnalogOutputDevice::scheduledStateAt(const ScheduledAnalogOutputDeviceConfigV2& config, const uint16_t minuteOfDay,
                                                       bool& hasPoints) {
    const ScheduledAnalogOutputPointV1* previous = nullptr;
    const ScheduledAnalogOutputPointV1* next = nullptr;
    uint16_t previousDistance = kAnalogScheduleMinutesPerDay;
    uint16_t nextDistance = kAnalogScheduleMinutesPerDay;
    uint8_t activeCount = 0U;
    for (uint8_t index = 0U; index < kMaxScheduledAnalogOutputPoints; ++index) {
        const ScheduledAnalogOutputPointV1& point = config.points[index];
        if (point.deleted != 0U) {
            continue;
        }
        ++activeCount;
        const uint16_t backward =
            static_cast<uint16_t>((minuteOfDay + kAnalogScheduleMinutesPerDay - point.minuteOfDay) % kAnalogScheduleMinutesPerDay);
        const uint16_t forward =
            static_cast<uint16_t>((point.minuteOfDay + kAnalogScheduleMinutesPerDay - minuteOfDay) % kAnalogScheduleMinutesPerDay);
        if (backward < previousDistance) {
            previousDistance = backward;
            previous = &point;
        }
        if (forward < nextDistance) {
            nextDistance = forward;
            next = &point;
        }
    }
    hasPoints = activeCount > 0U;
    if (!hasPoints || previous == nullptr || next == nullptr) {
        return 0U;
    }
    if (activeCount == 1U || previousDistance + nextDistance == 0U) {
        return previous->state;
    }
    const int64_t delta = static_cast<int64_t>(next->state) - static_cast<int64_t>(previous->state);
    const int64_t span = static_cast<int64_t>(previousDistance + nextDistance);
    int64_t numerator = delta * previousDistance;
    numerator += numerator >= 0 ? span / 2 : -(span / 2);
    const int64_t result = static_cast<int64_t>(previous->state) + numerator / span;
    return static_cast<uint16_t>(std::max<int64_t>(0, std::min<int64_t>(kAnalogOutputLevelMax, result)));
}

DeviceTypeDescriptor ScheduledAnalogOutputDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kScheduledAnalogOutputDeviceTypeId;
    descriptor.name = "ScheduledAnalogOutputDevice";
    descriptor.currentConfigVersion = kScheduledAnalogOutputDeviceConfigVersion;
    descriptor.maxDependents = 8;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.ticks1s = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::AnalogOutput, true}};
    descriptor.exclusiveDependencyRoles = ProvidedRoles::of({DeviceRole::AnalogOutput});
    descriptor.providedRoles = ProvidedRoles::of({IAnalogOutputRuntime::kProvidedRole});
    descriptor.createRuntime = &ScheduledAnalogOutputDevice::createRuntime;
    descriptor.validateConfig = &ScheduledAnalogOutputDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> ScheduledAnalogOutputDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                           const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new ScheduledAnalogOutputDevice(record, configBlob));
}

DeviceValidationResult ScheduledAnalogOutputDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::AnalogOutput) == 0U) {
        return {DeviceError::InvalidRelationship, "analog output dependency is required"};
    }
    ScheduledAnalogOutputDeviceConfigV2 config{};
    if (!decodeScheduledAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "scheduled analog output config is invalid"};
    }
    return config.validate();
}

void ScheduledAnalogOutputDevice::onReadyTick(const uint32_t now) {
    if (mode_ == AnalogOutputMode::Off) {
        if (currentOutputState() != 0U) {
            (void)forwardOutputState(0U, now);
            markRuntimeStateDirty();
        }
    } else if (mode_ == AnalogOutputMode::Scheduled) {
        updateScheduledState(now, false);
    }
}

void ScheduledAnalogOutputDevice::onTargetAttached(const uint32_t now) {
    if (mode_ == AnalogOutputMode::Off) {
        (void)forwardOutputState(0U, now);
    } else if (mode_ == AnalogOutputMode::Manual) {
        (void)forwardOutputState(manualState_, now);
    } else {
        updateScheduledState(now, true);
    }
}

void ScheduledAnalogOutputDevice::updateScheduledState(const uint32_t now, const bool force) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    const DateTime current = DateTime::current();
    timeValid_ = current.year() >= kMinimumValidYear;
    const uint32_t minuteKey = timeValid_ ? current.unixtime() / 60U : 0U;
    if (!force && cacheValid_ && minuteKey == cachedMinuteKey_) {
        return;
    }
    bool hasPoints = false;
    scheduledState_ = timeValid_ ? scheduledStateAt(config_, current.minutesOfDay(), hasPoints) : 0U;
    if (!hasPoints) {
        scheduledState_ = 0U;
    }
    cachedMinuteKey_ = minuteKey;
    cacheValid_ = true;
#else
    (void)force;
    timeValid_ = false;
    scheduledState_ = 0U;
    cacheValid_ = true;
#endif
    if (targetOutput() != nullptr && targetOutput()->currentOutputState() != scheduledState_) {
        (void)forwardOutputState(scheduledState_, now);
        markRuntimeStateDirty();
    }
}

void ScheduledAnalogOutputDevice::setMode(const AnalogOutputMode mode) {
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;
    retainedStateDirty_ = true;
    markRuntimeStateDirty();
}

} // namespace ewfm
