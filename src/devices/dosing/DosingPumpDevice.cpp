#include "devices/dosing/DosingPumpDevice.h"

#include "devices/dosing/journal/DoseJournal.h"
#include "devices/registry/DeviceRetainedDataStore.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS DosingPumpDevice

namespace {
// Mirrors kScheduleMinValidYear/kAutoSwitchMinValidYear: never evaluate the dose schedule against
// a dead-battery/never-synced clock.
constexpr uint16_t kDosingPumpMinValidYear = 2020U;
constexpr uint32_t kSecondsPerDay = 86400UL;
constexpr uint32_t kScheduleEvalIntervalMs = 1000U;
constexpr uint32_t kRunProgressDirtyIntervalMs = 1000U;

uint32_t containerCapacityCentiMl(const DosingPumpDeviceConfigV1& config) {
    return static_cast<uint32_t>(config.containerCapacityMl) * 100UL;
}
} // namespace

DosingPumpDevice::DosingPumpDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : DosingPumpDevice([&configBlob]() {
          DosingPumpDeviceConfigV1 config{};
          (void)decodeValidatedFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

DosingPumpDevice::DosingPumpDevice(const DosingPumpDeviceConfigV1& config)
    : DeviceRuntimeBase((PState)&DosingPumpDevice::Idle), config_(config) {
    state_.containerCurrentCentiMl = containerCapacityCentiMl(config_);
}

const DosingPumpDeviceConfigV1& DosingPumpDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& DosingPumpDevice::baseConfig() const {
    return config_;
}

void DosingPumpDevice::setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntime(role, dependencyRuntime);
    refreshCapabilityCache();
}

void DosingPumpDevice::setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntimeAt(index, dependencyRuntime);
    refreshCapabilityCache();
}

void DosingPumpDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
    refreshCapabilityCache();
}

bool DosingPumpDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dosingPumpDeviceConfigSize(config_);
    return encodeFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan DosingPumpDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    DosingPumpDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    return {};
}

bool DosingPumpDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    DosingPumpDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    // Dose slots may have been reordered/replaced, so per-slot bitmasks are meaningless now.
    // firedMask self-heals on the next evaluation: doses already past their grace window get
    // re-marked as missed instead of re-dosed.
    state_.firedMask = 0U;
    state_.skipNextMask = 0U;
    const uint32_t capacity = containerCapacityCentiMl(config_);
    if (state_.containerCurrentCentiMl > capacity) {
        state_.containerCurrentCentiMl = capacity;
    }
    nextDoseValid_ = false;
    lastScheduleEvalMs_ = 0U;
    markRetainedDirty();
    markRuntimeStateDirty();
    return true;
}
bool DosingPumpDevice::handleCommand(const DeviceCommand& command) {
    if (command.type != DeviceCommandType::Custom) {
        return false;
    }
    const char* payload = command.payload.c_str();
    if (command.payload.equals("auto")) {
        setAutoMode(true);
        return true;
    }
    if (command.payload.equals("manual")) {
        setAutoMode(false);
        return true;
    }
    if (command.payload.equals("stopDose")) {
        return stopRun(uptime());
    }
    if (std::strncmp(payload, "startDose:", 10U) == 0) {
        unsigned long amountCentiMl = 0UL;
        unsigned int logging = 1U;
        if (std::sscanf(payload, "startDose:%lu:%u", &amountCentiMl, &logging) != 2 || amountCentiMl == 0UL || amountCentiMl > UINT16_MAX) {
            return false;
        }
        return startRun(logging != 0U ? DosingRunType::Manual : DosingRunType::Calibration, static_cast<uint16_t>(amountCentiMl), uptime());
    }
    if (std::strncmp(payload, "setVolume:", 10U) == 0) {
        unsigned long volumeCentiMl = 0UL;
        if (std::sscanf(payload, "setVolume:%lu", &volumeCentiMl) != 1) {
            return false;
        }
        const uint32_t capacity = containerCapacityCentiMl(config_);
        state_.containerCurrentCentiMl = volumeCentiMl > capacity ? capacity : static_cast<uint32_t>(volumeCentiMl);
        markRetainedDirty();
        markRuntimeStateDirty();
        return true;
    }
    if (std::strncmp(payload, "skipNext:", 9U) == 0) {
        unsigned int index = 0U;
        unsigned int skip = 0U;
        if (std::sscanf(payload, "skipNext:%u:%u", &index, &skip) != 2 || index >= config_.doseCount) {
            return false;
        }
        const uint16_t bit = static_cast<uint16_t>(1U << index);
        const uint16_t updated =
            skip != 0U ? static_cast<uint16_t>(state_.skipNextMask | bit) : static_cast<uint16_t>(state_.skipNextMask & ~bit);
        if (updated != state_.skipNextMask) {
            state_.skipNextMask = updated;
            nextDoseValid_ = false;
            lastScheduleEvalMs_ = 0U;
            markRetainedDirty();
            markRuntimeStateDirty();
        }
        return true;
    }
    return false;
}

bool DosingPumpDevice::retainedStateDirty() const {
    return retainedStateDirty_;
}

void DosingPumpDevice::clearRetainedStateDirty() {
    retainedStateDirty_ = false;
}

DeviceValidationResult DosingPumpDevice::saveRetainedState(DeviceRetainedDataStore& store) const {
    DosingPumpRetainedStateV1 record = state_;
    record.recordVersion = kRetainedStateRecordVersion;
    record.deviceId = deviceId();
    return store.save(record);
}

DeviceValidationResult DosingPumpDevice::loadRetainedState(DeviceRetainedDataStore& store) {
    DosingPumpRetainedStateV1 record{};
    const DeviceValidationResult result = store.load(deviceId(), record);
    if (!result.ok()) {
        return result;
    }
    record.autoMode = record.autoMode != 0U ? 1U : 0U;
    const uint16_t doseBits = config_.doseCount >= 16U ? 0xFFFFU : static_cast<uint16_t>((1U << config_.doseCount) - 1U);
    record.skipNextMask &= doseBits;
    record.firedMask &= doseBits;
    const uint32_t capacity = containerCapacityCentiMl(config_);
    if (record.containerCurrentCentiMl > capacity) {
        record.containerCurrentCentiMl = capacity;
    }
    state_ = record;
    return {};
}

void DosingPumpDevice::end(uint32_t now) {
    abortRunIfActive(now);
}

bool DosingPumpDevice::autoMode() const {
    return state_.autoMode != 0U;
}

bool DosingPumpDevice::timeValid() const {
    return timeValid_;
}

bool DosingPumpDevice::running() const {
    return runActive_;
}

DosingRunType DosingPumpDevice::runType() const {
    return runType_;
}

uint32_t DosingPumpDevice::runTargetCentiMl() const {
    return runActive_ ? runTargetCentiMl_ : 0U;
}

uint32_t DosingPumpDevice::runDosedCentiMl() const {
    if (!runActive_) {
        return 0U;
    }
    const uint32_t elapsed = uptime() - runStartMs_;
    const uint32_t dosed = dosedCentiMl(elapsed, config_.speedMilliMlPerSec);
    return dosed > runTargetCentiMl_ ? runTargetCentiMl_ : dosed;
}

uint32_t DosingPumpDevice::runRemainingMs() const {
    if (!runActive_) {
        return 0U;
    }
    const int32_t remaining = static_cast<int32_t>(runEndMs_ - uptime());
    return remaining > 0 ? static_cast<uint32_t>(remaining) : 0U;
}

uint32_t DosingPumpDevice::runTotalMs() const {
    return runActive_ ? runEndMs_ - runStartMs_ : 0U;
}

uint32_t DosingPumpDevice::lastRunDosedCentiMl() const {
    return lastRunDosedCentiMl_;
}

uint32_t DosingPumpDevice::todayDosedCentiMl() const {
    return state_.todayDosedCentiMl;
}

uint32_t DosingPumpDevice::todayTargetCentiMlCached() const {
    return todayTargetCentiMl_;
}

bool DosingPumpDevice::nextDoseAt(uint32_t& outEpoch, uint16_t& outAmountCentiMl) const {
    if (!nextDoseValid_) {
        return false;
    }
    outEpoch = nextDoseEpoch_;
    outAmountCentiMl = nextDoseAmountCentiMl_;
    return true;
}

uint32_t DosingPumpDevice::lastDoseEpoch() const {
    return state_.lastDoseEpoch;
}

DosingRunType DosingPumpDevice::lastDoseType() const {
    return static_cast<DosingRunType>(state_.lastDoseType);
}

uint16_t DosingPumpDevice::lastDoseAmountCentiMl() const {
    return state_.lastDoseAmountCentiMl;
}

uint32_t DosingPumpDevice::containerCurrentCentiMl() const {
    return state_.containerCurrentCentiMl;
}

bool DosingPumpDevice::sensorReportsEmpty() const {
    return levelSensor_ != nullptr && levelSensor_->isActive() != levelSensorInvert_;
}

bool DosingPumpDevice::containerEmpty() const {
    return sensorReportsEmpty() || state_.containerCurrentCentiMl == 0U;
}

bool DosingPumpDevice::levelSensorPresent() const {
    return levelSensor_ != nullptr;
}

uint16_t DosingPumpDevice::skipNextMask() const {
    return state_.skipNextMask;
}

bool DosingPumpDevice::daysLeft(uint32_t& outDays) const {
    const uint32_t averageDaily = averageDailyCentiMl(config_);
    if (averageDaily == 0U) {
        return false;
    }
    outDays = state_.containerCurrentCentiMl / averageDaily;
    return true;
}

void DosingPumpDevice::evaluateSchedule(const DateTime& nowWall, uint32_t nowUptimeMs) {
    if (nowWall.year() < kDosingPumpMinValidYear) {
        timeValid_ = false;
        nextDoseValid_ = false;
        return;
    }
    timeValid_ = true;
    currentWallEpoch_ = nowWall.unixtime();

    const uint32_t localDay = currentWallEpoch_ / kSecondsPerDay;
    const uint16_t minuteOfDay = nowWall.minutesOfDay();

    if (state_.dayNumber != localDay) {
        state_.dayNumber = localDay;
        state_.firedMask = 0U;
        state_.todayDosedCentiMl = 0U;
        markRetainedDirty();
        markRuntimeStateDirty();
    }

    const bool dayActive = dosingDayActive(config_, localDay);
    if (dayActive) {
        // Drop-don't-dose-late: anything past its grace window (boot catch-up, long manual run,
        // clock arriving mid-day) is marked handled without running.
        const uint16_t missed = static_cast<uint16_t>(missedDosesMask(config_, minuteOfDay) & ~state_.firedMask);
        if (missed != 0U) {
            state_.firedMask |= missed;
            markRetainedDirty();
        }

        if (autoMode() && !runActive_ && status_ == DeviceStatus::Ready) {
            const int dueIndex = dueDoseIndex(config_, minuteOfDay, state_.firedMask);
            if (dueIndex >= 0) {
                const uint16_t doseBit = static_cast<uint16_t>(1U << dueIndex);
                if ((state_.skipNextMask & doseBit) != 0U) {
                    // Consume the one-shot skip: handled for today, armed again for the next
                    // occurrence only if the user re-enables it.
                    state_.firedMask |= doseBit;
                    state_.skipNextMask = static_cast<uint16_t>(state_.skipNextMask & ~doseBit);
                    markRetainedDirty();
                    markRuntimeStateDirty();
                } else if (containerEmpty() && config_.blockAutoWhenEmpty != 0U) {
                    // Dropped, not deferred: an empty container is not going to refill within the
                    // grace window, and a late burst after a refill would be worse.
                    state_.firedMask |= doseBit;
                    markRetainedDirty();
                    markRuntimeStateDirty();
                } else {
                    // The fired bit is set inside startRun (at start, not completion) - and also on
                    // failure, so a misbehaving switch cannot cause a retry storm.
                    const uint16_t amount = config_.doses[dueIndex].amountCentiMl;
                    state_.firedMask |= doseBit;
                    markRetainedDirty();
                    (void)startRun(DosingRunType::Scheduled, amount, nowUptimeMs);
                }
            }
        }
    }

    refreshDerivedSchedule(localDay, minuteOfDay);
    scheduleEvaluatedOnce_ = true;
}

bool DosingPumpDevice::startRun(DosingRunType type, uint16_t amountCentiMl, uint32_t now) {
    if (runActive_ || status_ != DeviceStatus::Ready || pumpSwitch_ == nullptr) {
        return false;
    }
    if (amountCentiMl == 0U || config_.speedMilliMlPerSec == 0U) {
        return false;
    }
    if (type == DosingRunType::Scheduled && containerEmpty() && config_.blockAutoWhenEmpty != 0U) {
        return false;
    }
    const uint32_t runMs = doseRunMs(amountCentiMl, config_.speedMilliMlPerSec);
    if (runMs == 0U) {
        return false;
    }
    if (!pumpSwitch_->requestOutputState(true, now)) {
        return false;
    }
    runActive_ = true;
    runType_ = type;
    runTargetCentiMl_ = amountCentiMl;
    runStartMs_ = now;
    runEndMs_ = now + runMs;
    lastProgressDirtyMs_ = now;
    markRuntimeStateDirty();
    return true;
}

bool DosingPumpDevice::stopRun(uint32_t now) {
    if (!runActive_) {
        return true;
    }
    finishRun(now);
    return true;
}

DeviceTypeDescriptor DosingPumpDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kDosingPumpDeviceTypeId;
    descriptor.name = "DosingPumpDevice";
    descriptor.currentConfigVersion = kDosingPumpDeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::Switch, true},
                                         DeviceDependencyRequirement{DeviceRole::Condition, false}};
    descriptor.createRuntime = &DosingPumpDevice::createRuntime;
    descriptor.validateConfig = &DosingPumpDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> DosingPumpDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new DosingPumpDevice(record, configBlob));
}

DeviceValidationResult DosingPumpDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::Switch) == 0U) {
        return {DeviceError::InvalidRelationship, "dosing pump requires a switch dependency"};
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "dosing pump config exceeds supported size"};
    }
    DosingPumpDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "dosing pump config is invalid"};
    }
    return config.validate();
}

void DosingPumpDevice::refreshCapabilityCache() {
    pumpSwitch_ = nullptr;
    levelSensor_ = nullptr;
    levelSensorInvert_ = false;
    const DeviceDependencyLink* links = dependencyLinks();
    const uint8_t count = dependencyCount();
    for (uint8_t index = 0; index < count && links != nullptr; ++index) {
        IDeviceRuntime* dependencyRuntime = dependencyRuntimeAt(index);
        if (dependencyRuntime == nullptr) {
            continue;
        }
        if (links[index].role == DeviceRole::Switch) {
            pumpSwitch_ = const_cast<ISwitchOutputRuntime*>(dependencyRuntime->switchOutputRuntime());
        } else if (links[index].role == DeviceRole::Condition && levelSensor_ == nullptr) {
            levelSensor_ = dependencyRuntime->statusRuntime();
            levelSensorInvert_ = links[index].invert;
        }
    }
}

bool DosingPumpDevice::dependenciesAvailable() const {
    return dependencyRuntime(DeviceRole::Switch) != nullptr && pumpSwitch_ != nullptr;
}

bool DosingPumpDevice::dependencyBlocked() const {
    const IDeviceRuntime* switchRuntime = dependencyRuntime(DeviceRole::Switch);
    return switchRuntime == nullptr || pumpSwitch_ == nullptr || switchRuntime->status() != DeviceStatus::Ready;
}

bool DosingPumpDevice::dependencyIsDisabled() const {
    const IDeviceRuntime* switchRuntime = dependencyRuntime(DeviceRole::Switch);
    return switchRuntime != nullptr && switchRuntime->status() == DeviceStatus::Disabled;
}

void DosingPumpDevice::setAutoMode(bool enabledMode) {
    const uint8_t value = enabledMode ? 1U : 0U;
    if (state_.autoMode == value) {
        return;
    }
    state_.autoMode = value;
    markRetainedDirty();
    markRuntimeStateDirty();
}

void DosingPumpDevice::finishRun(uint32_t now) {
    const uint32_t endMs = EWFM_SM_TIME_REACHED(now, runEndMs_) ? runEndMs_ : now;
    const uint32_t elapsed = endMs - runStartMs_;
    uint32_t actual = dosedCentiMl(elapsed, config_.speedMilliMlPerSec);
    if (actual > runTargetCentiMl_) {
        actual = runTargetCentiMl_;
    }
    if (pumpSwitch_ != nullptr) {
        (void)pumpSwitch_->requestOutputState(false, now);
    }
    runActive_ = false;
    lastRunDosedCentiMl_ = actual;

    state_.containerCurrentCentiMl = state_.containerCurrentCentiMl > actual ? state_.containerCurrentCentiMl - actual : 0U;
    if (runType_ != DosingRunType::Calibration) {
        state_.todayDosedCentiMl += actual;
        state_.lastDoseType = static_cast<uint8_t>(runType_);
        state_.lastDoseAmountCentiMl = actual > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(actual);
        state_.lastDoseEpoch = timeValid_ ? currentWallEpoch_ : 0U;
        // Journal writes must never gate dosing: missing journal or invalid clock just means no
        // history record (totals above still update).
        IDoseJournal* journal = defaultDoseJournal();
        if (journal != nullptr && timeValid_ && actual > 0U) {
            DoseJournalRecordV1 record{};
            record.epoch = currentWallEpoch_;
            record.deviceId = deviceId();
            record.type =
                static_cast<uint8_t>(runType_ == DosingRunType::Scheduled ? DoseJournalEntryType::Schedule : DoseJournalEntryType::Manual);
            record.amountCentiMl = state_.lastDoseAmountCentiMl;
            (void)journal->append(record);
        }
    }
    markRetainedDirty();
    markRuntimeStateDirty();
}

void DosingPumpDevice::abortRunIfActive(uint32_t now) {
    if (runActive_) {
        finishRun(now);
    } else if (pumpSwitch_ != nullptr) {
        // Defensive: never leave the motor running when the device leaves Ready.
        if (pumpSwitch_->currentOutputState()) {
            (void)pumpSwitch_->requestOutputState(false, now);
        }
    }
}

void DosingPumpDevice::tickReady(uint32_t now) {
    if (runActive_ && EWFM_SM_TIME_REACHED(now, runEndMs_)) {
        finishRun(now);
    }
    if (runActive_ && static_cast<uint32_t>(now - lastProgressDirtyMs_) >= kRunProgressDirtyIntervalMs) {
        lastProgressDirtyMs_ = now;
        markRuntimeStateDirty();
    }
    if (lastScheduleEvalMs_ == 0U || static_cast<uint32_t>(now - lastScheduleEvalMs_) >= kScheduleEvalIntervalMs) {
        lastScheduleEvalMs_ = now == 0U ? 1U : now;
        evaluateScheduleFromClock(now);
    }
}

void DosingPumpDevice::evaluateScheduleFromClock(uint32_t now) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    evaluateSchedule(DateTime::current(), now);
#else
    (void)now;
#endif
}

void DosingPumpDevice::refreshDerivedSchedule(uint32_t localDayNumber, uint16_t nowMinuteOfDay) {
    const bool dayActive = dosingDayActive(config_, localDayNumber);
    const uint32_t target = todayTargetCentiMl(config_, dayActive);
    if (target != todayTargetCentiMl_) {
        todayTargetCentiMl_ = target;
        markRuntimeStateDirty();
    }

    uint32_t epoch = 0U;
    uint16_t amount = 0U;
    const bool hasNext = nextDose(config_, localDayNumber, nowMinuteOfDay, state_.firedMask, state_.skipNextMask, epoch, amount);
    if (hasNext != nextDoseValid_ || (hasNext && (epoch != nextDoseEpoch_ || amount != nextDoseAmountCentiMl_))) {
        nextDoseValid_ = hasNext;
        nextDoseEpoch_ = epoch;
        nextDoseAmountCentiMl_ = amount;
        markRuntimeStateDirty();
    }
}

void DosingPumpDevice::markRetainedDirty() {
    retainedStateDirty_ = true;
}

SM_STATE(DosingPumpDevice::Idle) {
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

SM_STATE(DosingPumpDevice::Starting) {
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

SM_STATE(DosingPumpDevice::Reconfiguring) {
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

SM_STATE(DosingPumpDevice::Ready) {
    status_ = DeviceStatus::Ready;
    const uint32_t now = uptime();
    if (deleteRequested_) {
        abortRunIfActive(now);
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        abortRunIfActive(now);
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        abortRunIfActive(now);
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        abortRunIfActive(now);
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        abortRunIfActive(now);
        SM_GOTO(Reconfiguring);
    }

    tickReady(now);
}

SM_STATE(DosingPumpDevice::DependencyBlocked) {
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

SM_STATE(DosingPumpDevice::Disabled) {
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

SM_STATE(DosingPumpDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
