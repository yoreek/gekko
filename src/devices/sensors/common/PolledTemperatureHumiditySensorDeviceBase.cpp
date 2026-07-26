#include "devices/sensors/common/PolledTemperatureHumiditySensorDeviceBase.h"

#include <cmath>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS PolledTemperatureHumiditySensorDeviceBase

namespace {
const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
} // namespace

PolledTemperatureHumiditySensorDeviceBase::PolledTemperatureHumiditySensorDeviceBase()
    : DeviceRuntimeBase((PState)&PolledTemperatureHumiditySensorDeviceBase::Idle) {}

const TemperatureReading& PolledTemperatureHumiditySensorDeviceBase::temperatureReading() const {
    return temperaturePublisher_.reading();
}

const HumidityReading& PolledTemperatureHumiditySensorDeviceBase::humidityReading() const {
    return humidityPublisher_.reading();
}

bool PolledTemperatureHumiditySensorDeviceBase::latestTemperatureReading(TemperatureReading& reading) const {
    reading = temperaturePublisher_.reading();
    return true;
}

const char* PolledTemperatureHumiditySensorDeviceBase::latestTemperatureStatus() const {
    return temperaturePublisher_.status();
}

bool PolledTemperatureHumiditySensorDeviceBase::latestHumidityReading(HumidityReading& reading) const {
    reading = humidityPublisher_.reading();
    return true;
}

const char* PolledTemperatureHumiditySensorDeviceBase::latestHumidityStatus() const {
    return humidityPublisher_.status();
}

const ITemperatureReadingRuntime* PolledTemperatureHumiditySensorDeviceBase::temperatureReadingRuntime() const {
    return this;
}

const IHumidityReadingRuntime* PolledTemperatureHumiditySensorDeviceBase::humidityReadingRuntime() const {
    return this;
}

void PolledTemperatureHumiditySensorDeviceBase::applyFilterConfig() {
    temperatureFilter_.configure(temperatureFilterConfig());
    humidityFilter_.configure(humidityFilterConfig());
}

void PolledTemperatureHumiditySensorDeviceBase::reschedulePoll(uint32_t now) {
    nextPollAt_ = now + pollIntervalMs();
}

void PolledTemperatureHumiditySensorDeviceBase::publishReadings(int32_t rawMilliCelsius, int32_t rawMilliPercent, uint32_t now) {
    temperaturePublisher_.configure(reportAlways(), reportDeltaCentiCelsius());
    humidityPublisher_.configure(reportAlways(), reportDeltaCentiPercent());

    const float filteredTemperature = temperatureFilter_.apply(static_cast<float>(rawMilliCelsius));
    float filteredHumidity = humidityFilter_.apply(static_cast<float>(rawMilliPercent));
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

void PolledTemperatureHumiditySensorDeviceBase::invalidateReadings(const char* status) {
    bool dirty = temperaturePublisher_.invalidate(status);
    dirty = humidityPublisher_.invalidate(status) || dirty;
    if (dirty) {
        markRuntimeStateDirty();
    }
}

void PolledTemperatureHumiditySensorDeviceBase::recordFailure(uint32_t now) {
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + (consecutiveErrors_ >= 3U ? 30000U : 1000U);
}

void PolledTemperatureHumiditySensorDeviceBase::performReading(uint32_t now) {
    int32_t rawMilliCelsius = 0;
    int32_t rawMilliPercent = 0;
    const char* invalidStatus = kOutputNotReady;
    if (!sampleReading(now, rawMilliCelsius, rawMilliPercent, invalidStatus)) {
        invalidateReadings(invalidStatus);
        recordFailure(now);
        if (consecutiveErrors_ >= 3U) {
            status_ = DeviceStatus::Faulted;
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }
    consecutiveErrors_ = 0U;
    publishReadings(rawMilliCelsius, rawMilliPercent, now);
}

SM_STATE(PolledTemperatureHumiditySensorDeviceBase::Idle) {
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

SM_STATE(PolledTemperatureHumiditySensorDeviceBase::Starting) {
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
    clearStartRequested();
    consecutiveErrors_ = 0U;
    applyFilterConfig();
    nextPollAt_ = uptime() + startDelayMs();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(PolledTemperatureHumiditySensorDeviceBase::Ready) {
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

SM_STATE(PolledTemperatureHumiditySensorDeviceBase::RetryBackoff) {
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
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (EWFM_SM_TIME_REACHED(uptime(), retryDeadline_)) {
        SM_GOTO(Starting);
    }
}

SM_STATE(PolledTemperatureHumiditySensorDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    invalidateReadings(kOutputNotReady);
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

SM_STATE(PolledTemperatureHumiditySensorDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    invalidateReadings(kOutputDisabled);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_ && enabled()) {
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(PolledTemperatureHumiditySensorDeviceBase::Faulted) {
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
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (EWFM_SM_TIME_REACHED(uptime(), retryDeadline_)) {
        SM_GOTO(Starting);
    }
}

SM_STATE(PolledTemperatureHumiditySensorDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    invalidateReadings(kOutputNotReady);
    setDeleted();
}

} // namespace ewfm
