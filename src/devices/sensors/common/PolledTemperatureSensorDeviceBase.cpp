#include "devices/sensors/common/PolledTemperatureSensorDeviceBase.h"

#include <cmath>
#include <cstdint>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS PolledTemperatureSensorDeviceBase

namespace {
const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
const char* kOutputDependencyUnavailable = "dependency_unavailable";
} // namespace

PolledTemperatureSensorDeviceBase::PolledTemperatureSensorDeviceBase()
    : DeviceRuntimeBase((PState)&PolledTemperatureSensorDeviceBase::Idle) {}

const TemperatureReading& PolledTemperatureSensorDeviceBase::reading() const {
    return publisher_.reading();
}

const char* PolledTemperatureSensorDeviceBase::outputStatus() const {
    return publisher_.status();
}

bool PolledTemperatureSensorDeviceBase::latestTemperatureReading(TemperatureReading& reading) const {
    reading = publisher_.reading();
    return true;
}

const char* PolledTemperatureSensorDeviceBase::latestTemperatureStatus() const {
    return publisher_.status();
}

const ITemperatureReadingRuntime* PolledTemperatureSensorDeviceBase::temperatureReadingRuntime() const {
    return this;
}

void PolledTemperatureSensorDeviceBase::applyFilterConfig() {
    filter_.configure(filterConfig());
}

void PolledTemperatureSensorDeviceBase::reschedulePoll(uint32_t now) {
    nextPollAt_ = now + pollIntervalMs();
}

void PolledTemperatureSensorDeviceBase::performReading(uint32_t now) {
    int32_t rawMilliCelsius = 0;
    const char* invalidStatus = kOutputNotReady;
    if (!sampleReading(now, rawMilliCelsius, invalidStatus)) {
        if (publisher_.invalidate(invalidStatus)) {
            markRuntimeStateDirty();
        }
        return;
    }
    const float filtered = filter_.apply(static_cast<float>(rawMilliCelsius));
    publisher_.configure(reportAlways(), reportDeltaCentiCelsius());
    if (publisher_.publish(static_cast<int32_t>(std::lround(filtered)), now)) {
        markRuntimeStateDirty();
    }
}

SM_STATE(PolledTemperatureSensorDeviceBase::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        if (publisher_.invalidate(kOutputDisabled)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Starting);
    }
}

SM_STATE(PolledTemperatureSensorDeviceBase::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        if (publisher_.invalidate(kOutputDisabled)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Disabled);
    }
    if (!sensorReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        if (publisher_.invalidate(kOutputDependencyUnavailable)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(DependencyBlocked);
    }

    clearStartRequested();
    filter_.reset();
    nextPollAt_ = uptime();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(PolledTemperatureSensorDeviceBase::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        if (publisher_.invalidate(kOutputDisabled)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Disabled);
    }
    if (!sensorReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        if (publisher_.invalidate(kOutputDependencyUnavailable)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), nextPollAt_)) {
        return;
    }
    nextPollAt_ = uptime() + pollIntervalMs();
    performReading(uptime());
}

SM_STATE(PolledTemperatureSensorDeviceBase::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        if (publisher_.invalidate(kOutputDisabled)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Disabled);
    }
    if (sensorReady() && (reconfigureRequested_ || startRequested_)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(PolledTemperatureSensorDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    if (publisher_.invalidate(kOutputNotReady)) {
        markRuntimeStateDirty();
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !enabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    SM_GOTO(Starting);
}

// Entry action: publish "disabled" once, then park. A device can stay disabled
// indefinitely, so keep invalidate() off the per-tick path.
SM_STATE(PolledTemperatureSensorDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (publisher_.invalidate(kOutputDisabled)) {
        markRuntimeStateDirty();
    }
    SM_GOTO(DisabledParked);
}

// Parked: no side effects, only the transitions out of Disabled.
SM_STATE(PolledTemperatureSensorDeviceBase::DisabledParked) {
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

// Entry action: publish the terminal status and mark deleted once, then park in
// Deleted so the registry can reap the runtime without re-running side effects.
SM_STATE(PolledTemperatureSensorDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    if (publisher_.invalidate(kOutputNotReady)) {
        markRuntimeStateDirty();
    }
    setDeleted();
    SM_GOTO(Deleted);
}

// Terminal parked state: does nothing, waiting to be reaped.
SM_STATE(PolledTemperatureSensorDeviceBase::Deleted) {}

} // namespace ewfm
