#include "devices/switch/auto/AutoSwitchDevice.h"

#include "devices/registry/DeviceRetainedDataStore.h"
#include "time/DateTime.h"

#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS AutoSwitchDevice

namespace {
// Guards the reboot-recovery wall-clock math below against a dead-battery RTC or a never-synced
// system clock, which commonly reads back as some power-on-reset/epoch-zero date - mirrors
// ScheduleDevice's kScheduleMinValidYear.
constexpr uint16_t kAutoSwitchMinValidYear = 2020U;

uint32_t remainingMsUntil(uint32_t now, uint32_t deadline) {
    const int32_t remaining = static_cast<int32_t>(deadline - now);
    return remaining > 0 ? static_cast<uint32_t>(remaining) : 0U;
}
} // namespace

uint32_t AutoSwitchDevice::computePausedUntilEpoch(uint32_t nowUptimeMs, uint32_t pausedUntilMs, uint32_t nowWallEpoch) {
    const uint32_t remainingSeconds = remainingMsUntil(nowUptimeMs, pausedUntilMs) / 1000U;
    return nowWallEpoch + remainingSeconds;
}

bool AutoSwitchDevice::recoverPausedUntilMs(uint32_t pausedUntilEpoch, uint32_t nowWallEpoch, uint32_t nowUptimeMs,
                                            uint32_t& outPausedUntilMs) {
    if (pausedUntilEpoch <= nowWallEpoch) {
        return false;
    }
    const uint32_t remainingSeconds = pausedUntilEpoch - nowWallEpoch;
    outPausedUntilMs = nowUptimeMs + remainingSeconds * 1000UL;
    return true;
}

AutoSwitchDevice::AutoSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : AutoSwitchDevice([&configBlob]() {
          AutoSwitchDeviceConfigV1 config{};
          (void)decodeValidatedFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

AutoSwitchDevice::AutoSwitchDevice(const AutoSwitchDeviceConfigV1& config)
    : DeviceRuntimeBase((PState)&AutoSwitchDevice::Idle), config_(config) {}

const AutoSwitchDeviceConfigV1& AutoSwitchDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& AutoSwitchDevice::baseConfig() const {
    return config_;
}

void AutoSwitchDevice::setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntime(role, dependencyRuntime);
    refreshCapabilityCache();
}

void AutoSwitchDevice::setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntimeAt(index, dependencyRuntime);
    refreshCapabilityCache();
}

void AutoSwitchDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
    refreshCapabilityCache();
}

bool AutoSwitchDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = autoSwitchDeviceConfigSize(config_);
    return encodeFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan AutoSwitchDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    AutoSwitchDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    return {};
}

bool AutoSwitchDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    AutoSwitchDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}
bool AutoSwitchDevice::handleCommand(const DeviceCommand& command) {
    // "auto"/"pause" arrive via the portal's generic "setMode" command (routed to
    // DeviceCommandType::Custom, see DeviceRegistryController::command()) - "on"/"off" arrive via
    // the ordinary "setOutput" command below, shared with every other Switch-role device. There is
    // no separate "resume": mirrors ReefDuino's schedule(), which is the single entry point into
    // ScheduledMode from anywhere (including PausedMode) - "auto" is that same entry point here.
    if (command.type == DeviceCommandType::Custom && command.payload.equals("auto")) {
        setMode(AutoSwitchMode::Auto);
        return true;
    }
    if (command.type == DeviceCommandType::Custom && command.payload.equals("pause")) {
        // Mirrors ReefDuino's pause(), which only takes effect when already in ScheduledMode.
        if (mode_ != AutoSwitchMode::Auto) {
            return false;
        }
        pausedUntilMs_ = uptime() + config_.pauseDurationSeconds * 1000UL;
        setMode(AutoSwitchMode::Paused);
        return true;
    }

    StateType requested{};
    if (!parseSwitchOutputStateCommand(command, requested)) {
        return false;
    }
    if (status_ != DeviceStatus::Ready) {
        return false;
    }
    return requestOutputState(requested, uptime());
}

bool AutoSwitchDevice::retainedStateDirty() const {
    return retainedStateDirty_;
}

void AutoSwitchDevice::clearRetainedStateDirty() {
    retainedStateDirty_ = false;
}

DeviceValidationResult AutoSwitchDevice::saveRetainedState(DeviceRetainedDataStore& store) const {
    AutoSwitchRetainedStateV1 record{};
    record.recordVersion = kRetainedStateRecordVersion;
    record.deviceId = deviceId();
    record.mode = static_cast<uint8_t>(mode_);
    record.pausedUntilEpoch = 0U;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (mode_ == AutoSwitchMode::Paused) {
        const DateTime nowWall = DateTime::current();
        if (nowWall.year() >= kAutoSwitchMinValidYear) {
            // pausedUntilMs_ is uptime()-relative and meaningless after a reboot - anchor the
            // remaining duration to wall-clock time instead, so loadRetainedState() can recover it.
            record.pausedUntilEpoch = computePausedUntilEpoch(uptime(), pausedUntilMs_, nowWall.unixtime());
        }
    }
#endif
    return store.save(record);
}

DeviceValidationResult AutoSwitchDevice::loadRetainedState(DeviceRetainedDataStore& store) {
    AutoSwitchRetainedStateV1 record{};
    const DeviceValidationResult result = store.load(deviceId(), record);
    if (!result.ok()) {
        return result;
    }
    if (record.mode == static_cast<uint8_t>(AutoSwitchMode::Off) || record.mode == static_cast<uint8_t>(AutoSwitchMode::On) ||
        record.mode == static_cast<uint8_t>(AutoSwitchMode::Auto) || record.mode == static_cast<uint8_t>(AutoSwitchMode::Paused)) {
        mode_ = static_cast<AutoSwitchMode>(record.mode);
    }
    if (mode_ == AutoSwitchMode::Paused) {
        mode_ = AutoSwitchMode::Auto; // safe default unless the block below recovers a real deadline
#if defined(ARDUINO) && !defined(UNIT_TEST)
        const DateTime nowWall = DateTime::current();
        uint32_t recoveredPausedUntilMs = 0U;
        if (nowWall.year() >= kAutoSwitchMinValidYear &&
            recoverPausedUntilMs(record.pausedUntilEpoch, nowWall.unixtime(), uptime(), recoveredPausedUntilMs)) {
            pausedUntilMs_ = recoveredPausedUntilMs;
            mode_ = AutoSwitchMode::Paused;
        }
#endif
    }
    return {};
}

AutoSwitchDevice::StateType AutoSwitchDevice::currentOutputState() const {
    return targetSwitch_ != nullptr ? targetSwitch_->currentOutputState() : false;
}

bool AutoSwitchDevice::requestOutputState(const StateType state, uint32_t now) {
    setMode(state ? AutoSwitchMode::On : AutoSwitchMode::Off);
    if (targetSwitch_ == nullptr) {
        return true;
    }
    if (state == targetSwitch_->currentOutputState()) {
        return true;
    }
    return targetSwitch_->requestOutputState(state, now);
}

AutoSwitchMode AutoSwitchDevice::mode() const {
    return mode_;
}

bool AutoSwitchDevice::paused() const {
    return mode_ == AutoSwitchMode::Paused;
}

uint32_t AutoSwitchDevice::pausedUntilMs() const {
    return pausedUntilMs_;
}

bool AutoSwitchDevice::conditionsSatisfied() const {
    // Empty list -> not satisfied, preserving the pre-generalization safe default: Auto never turns
    // on until at least one condition is actually attached, rather than being vacuously true.
    if (conditionCount_ == 0U) {
        return false;
    }
    for (uint8_t index = 0; index < conditionCount_; ++index) {
        const ConditionSource& condition = conditions_[index];
        const bool active = condition.source != nullptr && condition.source->isActive();
        if (active == condition.invert) {
            return false;
        }
    }
    return true;
}

DeviceTypeDescriptor AutoSwitchDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kAutoSwitchDeviceTypeId;
    descriptor.name = "AutoSwitchDevice";
    descriptor.currentConfigVersion = kAutoSwitchDeviceConfigVersion;
    descriptor.maxDependents = 4;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks1s = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::Switch, true},
                                         DeviceDependencyRequirement{DeviceRole::Condition, false}};
    descriptor.providedRoles = ProvidedRoles::of({ISwitchOutputRuntime::kProvidedRole, IStatusRuntime::kProvidedRole});
    descriptor.createRuntime = &AutoSwitchDevice::createRuntime;
    descriptor.validateConfig = &AutoSwitchDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> AutoSwitchDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new AutoSwitchDevice(record, configBlob));
}

DeviceValidationResult AutoSwitchDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::Switch) == 0U) {
        return {DeviceError::InvalidRelationship, "auto switch requires a switch dependency"};
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "auto switch config exceeds supported size"};
    }
    AutoSwitchDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "auto switch config is invalid"};
    }
    return config.validate();
}

void AutoSwitchDevice::refreshCapabilityCache() {
    targetSwitch_ = nullptr;
    conditions_ = {};
    conditionCount_ = 0U;
    const DeviceDependencyLink* links = dependencyLinks();
    const uint8_t count = dependencyCount();
    for (uint8_t index = 0; index < count && links != nullptr; ++index) {
        IDeviceRuntime* dependencyRuntime = dependencyRuntimeAt(index);
        if (dependencyRuntime == nullptr) {
            continue;
        }
        if (links[index].role == DeviceRole::Switch) {
            targetSwitch_ = const_cast<ISwitchOutputRuntime*>(dependencyRuntime->switchOutputRuntime());
        } else if (links[index].role == DeviceRole::Condition && conditionCount_ < kMaxAutoSwitchConditions) {
            conditions_[conditionCount_++] = ConditionSource{dependencyRuntime->statusRuntime(), links[index].invert};
        }
    }
}

bool AutoSwitchDevice::dependenciesAvailable() const {
    return dependencyRuntime(DeviceRole::Switch) != nullptr && targetSwitch_ != nullptr;
}

bool AutoSwitchDevice::dependencyBlocked() const {
    const IDeviceRuntime* switchRuntime = dependencyRuntime(DeviceRole::Switch);
    return switchRuntime == nullptr || targetSwitch_ == nullptr || switchRuntime->status() != DeviceStatus::Ready;
}

bool AutoSwitchDevice::dependencyIsDisabled() const {
    const IDeviceRuntime* switchRuntime = dependencyRuntime(DeviceRole::Switch);
    return switchRuntime != nullptr && switchRuntime->status() == DeviceStatus::Disabled;
}

AutoSwitchDevice::StateType AutoSwitchDevice::desiredOutputState() const {
    switch (mode_) {
    case AutoSwitchMode::Off:
        return false;
    case AutoSwitchMode::On:
        return true;
    case AutoSwitchMode::Paused:
        // Mirrors ReefDuino's PausedMode, which always _toggle(false)s regardless of what Auto was
        // last driving - pause forces the target off, it does not hold whatever state was current.
        return false;
    case AutoSwitchMode::Auto:
        return conditionsSatisfied();
    }
    return false;
}

void AutoSwitchDevice::applyDesiredOutput(uint32_t now) {
    if (targetSwitch_ == nullptr) {
        return;
    }
    if (mode_ == AutoSwitchMode::Paused && EWFM_SM_TIME_REACHED(now, pausedUntilMs_)) {
        // Mirrors ReefDuino's CHECK_PAUSE, which always reverts to ScheduledMode (Paused is only
        // ever reached from Auto) - setMode() below flags forceOffPending_ for this transition.
        setMode(AutoSwitchMode::Auto);
    }
    if (forceOffPending_) {
        forceOffPending_ = false;
        // Force off on the tick we enter Auto or Paused and let the *next* tick's normal schedule
        // check decide whether to turn back on - mirrors ReefDuino's SWITCH_MODE, which always
        // _toggle(false)s when entering ScheduledMode/PausedMode rather than immediately following
        // whatever the schedule happens to read right now.
        if (targetSwitch_->currentOutputState()) {
            (void)targetSwitch_->requestOutputState(false, now);
            markRuntimeStateDirty();
        }
        return;
    }
    const StateType desired = desiredOutputState();
    if (desired != targetSwitch_->currentOutputState()) {
        (void)targetSwitch_->requestOutputState(desired, now);
        markRuntimeStateDirty();
    }
}

void AutoSwitchDevice::setMode(AutoSwitchMode mode) {
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;
    if (mode == AutoSwitchMode::Auto || mode == AutoSwitchMode::Paused) {
        forceOffPending_ = true;
    }
    if (mode != AutoSwitchMode::Paused) {
        pausedUntilMs_ = 0U;
    }
    retainedStateDirty_ = true;
    markRuntimeStateDirty();
}

SM_STATE(AutoSwitchDevice::Idle) {
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

SM_STATE(AutoSwitchDevice::Starting) {
    status_ = DeviceStatus::Starting;
    refreshCapabilityCache();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }

    clearStartRequested();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(AutoSwitchDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    refreshCapabilityCache();
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
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(AutoSwitchDevice::Ready) {
    status_ = DeviceStatus::Ready;
    const uint32_t now = uptime();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        SM_GOTO(Reconfiguring);
    }

    applyDesiredOutput(now);
}

SM_STATE(AutoSwitchDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if ((reconfigureRequested_ || startRequested_) && dependenciesAvailable() && !dependencyBlocked()) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(AutoSwitchDevice::Disabled) {
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

SM_STATE(AutoSwitchDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
