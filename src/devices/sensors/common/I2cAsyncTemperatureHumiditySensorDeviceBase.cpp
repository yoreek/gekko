#include "devices/sensors/common/I2cAsyncTemperatureHumiditySensorDeviceBase.h"

#include <cmath>
#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS I2cAsyncTemperatureHumiditySensorDeviceBase

namespace {
constexpr uint32_t kBusyRetryMs = 100;
constexpr uint32_t kRetryBackoffMs = 1000;
constexpr uint32_t kFaultRetryBackoffMs = 30000;
constexpr uint8_t kFaultErrorThreshold = 3;

const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
const char* kOutputDependencyUnavailable = "dependency_unavailable";
const char* kOutputNotFound = "not_found";
const char* kBusyStatus = "busy";
} // namespace

I2cAsyncTemperatureHumiditySensorDeviceBase::I2cAsyncTemperatureHumiditySensorDeviceBase()
    : BusDependentDeviceBase((PState)&I2cAsyncTemperatureHumiditySensorDeviceBase::Idle) {}

bool I2cAsyncTemperatureHumiditySensorDeviceBase::latestTemperatureReading(TemperatureReading& reading) const {
    reading = temperaturePublisher_.reading();
    return true;
}

const char* I2cAsyncTemperatureHumiditySensorDeviceBase::latestTemperatureStatus() const {
    return temperaturePublisher_.status();
}

bool I2cAsyncTemperatureHumiditySensorDeviceBase::latestHumidityReading(HumidityReading& reading) const {
    reading = humidityPublisher_.reading();
    return true;
}

const char* I2cAsyncTemperatureHumiditySensorDeviceBase::latestHumidityStatus() const {
    return humidityPublisher_.status();
}

const ITemperatureReadingRuntime* I2cAsyncTemperatureHumiditySensorDeviceBase::temperatureReadingRuntime() const {
    return this;
}

const IHumidityReadingRuntime* I2cAsyncTemperatureHumiditySensorDeviceBase::humidityReadingRuntime() const {
    return this;
}

void I2cAsyncTemperatureHumiditySensorDeviceBase::applyFilterConfig() {
    temperatureFilter_.configure(temperatureFilterConfig());
    humidityFilter_.configure(humidityFilterConfig());
}

void I2cAsyncTemperatureHumiditySensorDeviceBase::reschedulePoll(uint32_t now) {
    nextPollAt_ = now + pollIntervalMs();
}

void I2cAsyncTemperatureHumiditySensorDeviceBase::publishReadings(int32_t rawMilliCelsius, int32_t rawMilliPercent, uint32_t now) {
    temperaturePublisher_.configure(reportAlways(), reportDeltaCentiCelsius());
    humidityPublisher_.configure(reportAlways(), reportDeltaCentiPercent());

    const float filteredTemperature = temperatureFilter_.apply(static_cast<float>(rawMilliCelsius));
    float filteredHumidity = humidityFilter_.apply(static_cast<float>(rawMilliPercent));
    // Calibration can push the filtered value outside the physical range - clamp after filtering.
    if (filteredHumidity < 0.0F) {
        filteredHumidity = 0.0F;
    } else if (filteredHumidity > 100000.0F) {
        filteredHumidity = 100000.0F;
    }

    bool dirty = temperaturePublisher_.publish(static_cast<int32_t>(std::lround(filteredTemperature)), now);
    dirty = humidityPublisher_.publish(static_cast<int32_t>(std::lround(filteredHumidity)), now) || dirty;
    if (dirty) {
        markRuntimeStateDirty();
    }
}

void I2cAsyncTemperatureHumiditySensorDeviceBase::invalidateReadings(const char* status) {
    bool dirty = temperaturePublisher_.invalidate(status);
    dirty = humidityPublisher_.invalidate(status) || dirty;
    if (dirty) {
        markRuntimeStateDirty();
    }
}

void I2cAsyncTemperatureHumiditySensorDeviceBase::recordFailure(uint32_t now) {
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + (consecutiveErrors_ >= kFaultErrorThreshold ? kFaultRetryBackoffMs : kRetryBackoffMs);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Starting);
    }
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }

    I2cBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    if (!sendInitCommand(*transaction.driver())) {
        invalidateReadings(kOutputNotFound);
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    lastDependencyGeneration_ = transaction.generation();
    clearStartRequested();
    temperatureFilter_.reset();
    humidityFilter_.reset();
    phaseDeadline_ = uptime() + initDelayMs();
    SM_GOTO(InitDelay);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::InitDelay) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), phaseDeadline_)) {
        return;
    }
    currentPhase_ = 0U;
    SM_GOTO(TriggerPhase);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::TriggerPhase) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }

    I2cBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    if (!sendPhaseTrigger(*transaction.driver(), currentPhase_)) {
        invalidateReadings(kOutputNotFound);
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    phaseDeadline_ = uptime() + phaseDelayMs(currentPhase_);
    SM_GOTO(ReadPhase);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::ReadPhase) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), phaseDeadline_)) {
        return;
    }

    I2cBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    const char* errorStatus = kOutputNotFound;
    if (!readPhase(*transaction.driver(), currentPhase_, errorStatus)) {
        invalidateReadings(errorStatus);
        if (errorStatus != nullptr && std::strcmp(errorStatus, kBusyStatus) == 0) {
            retryDeadline_ = uptime() + kBusyRetryMs;
        } else {
            recordFailure(uptime());
            if (consecutiveErrors_ >= kFaultErrorThreshold) {
                SM_GOTO(Faulted);
            }
        }
        SM_GOTO(RetryBackoff);
    }

    ++currentPhase_;
    if (currentPhase_ < measurementPhaseCount()) {
        SM_GOTO(TriggerPhase);
    }

    int32_t milliCelsius = 0;
    int32_t milliPercent = 0;
    computeReadings(milliCelsius, milliPercent);
    publishReadings(milliCelsius, milliPercent, uptime());
    consecutiveErrors_ = 0;
    nextPollAt_ = uptime() + pollIntervalMs();
    SM_GOTO(Ready);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), nextPollAt_)) {
        return;
    }
    currentPhase_ = 0U;
    SM_GOTO(TriggerPhase);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::RetryBackoff) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), retryDeadline_)) {
        return;
    }
    SM_GOTO(Starting);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (dependencyBusReady() && (reconfigureRequested_ || startRequested_ || lastDependencyGeneration_ == 0U)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    clearStartRequested();
    lastDependencyGeneration_ = 0;
    consecutiveErrors_ = 0;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    SM_GOTO(Starting);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    invalidateReadings(kOutputDisabled);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_ && enabled()) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        clearFaultRequested();
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), retryDeadline_)) {
        return;
    }
    SM_GOTO(Starting);
}

SM_STATE(I2cAsyncTemperatureHumiditySensorDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    invalidateReadings(kOutputNotReady);
    setDeleted();
}

} // namespace ewfm
