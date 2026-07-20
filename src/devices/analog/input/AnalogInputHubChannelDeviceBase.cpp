#include "devices/analog/input/AnalogInputHubChannelDeviceBase.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS AnalogInputHubChannelDeviceBase

namespace {
const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
const char* kOutputDependencyUnavailable = "dependency_unavailable";
} // namespace

AnalogInputHubChannelDeviceBase::AnalogInputHubChannelDeviceBase(PState initialState) : DeviceRuntimeBase(initialState) {}

const AnalogInputReading& AnalogInputHubChannelDeviceBase::reading() const {
    return publisher_.reading();
}

const char* AnalogInputHubChannelDeviceBase::outputStatus() const {
    return publisher_.status();
}

bool AnalogInputHubChannelDeviceBase::latestAnalogInputReading(AnalogInputReading& reading) const {
    reading = publisher_.reading();
    return true;
}

const char* AnalogInputHubChannelDeviceBase::latestAnalogInputStatus() const {
    return publisher_.status();
}

const IAnalogInputRuntime* AnalogInputHubChannelDeviceBase::analogInputRuntime() const {
    return this;
}

bool AnalogInputHubChannelDeviceBase::expanderChannel(uint8_t& channel) const {
    channel = this->channel();
    return true;
}

void AnalogInputHubChannelDeviceBase::end(uint32_t now) {
    (void)now;
    releaseHubClaim();
}

IAnalogInputHubRuntime* AnalogInputHubChannelDeviceBase::dependencyHub() const {
    IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::AnalogInputHub);
    if (dependency == nullptr) {
        return nullptr;
    }
    return const_cast<IAnalogInputHubRuntime*>(dependency->analogInputHubRuntime());
}

bool AnalogInputHubChannelDeviceBase::dependencyHubReady() const {
    return dependencyReady(DeviceRole::AnalogInputHub) && dependencyHub() != nullptr;
}

bool AnalogInputHubChannelDeviceBase::dependencyHubGenerationChanged() const {
    const IAnalogInputHubRuntime* hub = dependencyHub();
    return hub == nullptr || hub->generation() != lastHubGeneration_;
}

void AnalogInputHubChannelDeviceBase::releaseHubClaim() {
    IAnalogInputHubRuntime* hub = dependencyHub();
    if (hub != nullptr) {
        hub->releaseChannelRequest(channel(), deviceId());
    }
}

void AnalogInputHubChannelDeviceBase::recordFailure(uint32_t now) {
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + (consecutiveErrors_ >= kFaultErrorThreshold ? kFaultRetryBackoffMs : kRetryBackoffMs);
}

void AnalogInputHubChannelDeviceBase::invalidateAndMark(const char* status) {
    if (publisher_.invalidate(status)) {
        markRuntimeStateDirty();
    }
}

SM_STATE(AnalogInputHubChannelDeviceBase::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateAndMark(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !channelEnabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateAndMark(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        invalidateAndMark(kOutputNotReady);
        SM_GOTO(Starting);
    }
}

SM_STATE(AnalogInputHubChannelDeviceBase::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateAndMark(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !channelEnabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateAndMark(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyHubReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateAndMark(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }

    clearStartRequested();
    clearReconfigureRequested();
    lastHubGeneration_ = dependencyHub()->generation();
    consecutiveErrors_ = 0;
    accumulator_.reset(adcSampleCount());
    SM_GOTO(Sampling);
}

SM_STATE(AnalogInputHubChannelDeviceBase::Sampling) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHubClaim();
        invalidateAndMark(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !channelEnabled()) {
        status_ = DeviceStatus::Disabled;
        releaseHubClaim();
        invalidateAndMark(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyHubReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateAndMark(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyHubGenerationChanged() || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        releaseHubClaim();
        SM_GOTO(Reconfiguring);
    }

    AnalogInputReading sample{};
    const char* sampleStatus = nullptr;
    const AnalogInputHubPollResult result = dependencyHub()->pollChannelReading(channel(), deviceId(), uptime(), sample, sampleStatus);
    switch (result) {
    case AnalogInputHubPollResult::Busy:
    case AnalogInputHubPollResult::Pending:
        return;
    case AnalogInputHubPollResult::Fault:
        invalidateAndMark(sampleStatus != nullptr ? sampleStatus : "fault");
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    case AnalogInputHubPollResult::Ready:
        break;
    }

    accumulator_.add(sample.milliVolts);
    if (!accumulator_.complete()) {
        return;
    }

    consecutiveErrors_ = 0;
    publisher_.configure(reportAlways(), reportDeltaMilliVolts());
    if (publisher_.publish(sample.rawCode, accumulator_.average(), uptime())) {
        markRuntimeStateDirty();
    }
    nextPollAt_ = uptime() + pollIntervalMs();
    SM_GOTO(Ready);
}

SM_STATE(AnalogInputHubChannelDeviceBase::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateAndMark(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !channelEnabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateAndMark(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyHubReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateAndMark(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyHubGenerationChanged() || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), nextPollAt_)) {
        return;
    }
    accumulator_.reset(adcSampleCount());
    SM_GOTO(Sampling);
}

SM_STATE(AnalogInputHubChannelDeviceBase::RetryBackoff) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateAndMark(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !channelEnabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateAndMark(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyHubReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateAndMark(kOutputDependencyUnavailable);
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

SM_STATE(AnalogInputHubChannelDeviceBase::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateAndMark(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !channelEnabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateAndMark(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (dependencyHubReady() && (reconfigureRequested_ || startRequested_ || lastHubGeneration_ == 0U)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(AnalogInputHubChannelDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    clearStartRequested();
    lastHubGeneration_ = 0;
    consecutiveErrors_ = 0;
    invalidateAndMark(kOutputNotReady);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !channelEnabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyHubReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    SM_GOTO(Starting);
}

SM_STATE(AnalogInputHubChannelDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    invalidateAndMark(kOutputDisabled);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_ && channelEnabled()) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(AnalogInputHubChannelDeviceBase::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateAndMark(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || !channelEnabled()) {
        status_ = DeviceStatus::Disabled;
        invalidateAndMark(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyHubReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateAndMark(kOutputDependencyUnavailable);
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

SM_STATE(AnalogInputHubChannelDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    invalidateAndMark(kOutputNotReady);
    setDeleted();
}

} // namespace ewfm
