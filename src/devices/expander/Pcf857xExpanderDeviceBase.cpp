#include "devices/expander/Pcf857xExpanderDeviceBase.h"

#include "devices/bus/i2c/I2cDeviceValidation.h"

#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Pcf857xExpanderDeviceBase

namespace {
constexpr uint32_t kSyncIntervalMs = 10000;
constexpr uint32_t kBusyRetryMs = 100;
constexpr uint32_t kRetryBackoffMs = 1000;
constexpr uint32_t kFaultRetryBackoffMs = 30000;
constexpr uint8_t kFaultErrorThreshold = 3;
constexpr uint32_t kDiagnosticsDebounceMs = 1000;
} // namespace

Pcf857xExpanderDeviceBase::Pcf857xExpanderDeviceBase(const Pcf857xExpanderConfigV2& config)
    : Pcf857xExpanderRuntimeBase((PState)&Pcf857xExpanderDeviceBase::Idle), config_(config) {}

const Pcf857xExpanderConfigV2& Pcf857xExpanderDeviceBase::config() const {
    return config_;
}

const DeviceBaseConfigV1& Pcf857xExpanderDeviceBase::baseConfig() const {
    return config_;
}

bool Pcf857xExpanderDeviceBase::hasDuplicateDependentChannel(uint8_t channel, const IDeviceRuntime* ignoreDependent) const {
    constexpr uint8_t kMaxChannelsPerDependent = 8U;
    for (const IDeviceRuntime* dependent : dependentRuntimes()) {
        if (dependent == nullptr || dependent == ignoreDependent) {
            continue;
        }
        uint8_t dependentChannel{0};
        if (dependent->expanderChannel(dependentChannel) && dependentChannel == channel) {
            return true;
        }
        uint8_t channels[kMaxChannelsPerDependent]{};
        const uint8_t count = dependent->expanderChannels(channels, kMaxChannelsPerDependent);
        for (uint8_t i = 0; i < count; ++i) {
            if (channels[i] == channel) {
                return true;
            }
        }
    }
    return false;
}

const IPortExpanderRuntime* Pcf857xExpanderDeviceBase::portExpanderRuntime() const {
    return this;
}

uint8_t Pcf857xExpanderDeviceBase::channelCount() const {
    return channelCountImpl();
}

bool Pcf857xExpanderDeviceBase::isChannelOn(uint8_t channel) const {
    if (channel >= channelCountImpl()) {
        return false;
    }
    return (channelStates_ & (static_cast<uint32_t>(1) << channel)) != 0U;
}

bool Pcf857xExpanderDeviceBase::requestChannelState(uint8_t channel, bool on, uint32_t now) {
    if (channel >= channelCountImpl()) {
        return false;
    }

    if (on) {
        channelStates_ |= (static_cast<uint32_t>(1) << channel);
    } else {
        channelStates_ &= ~(static_cast<uint32_t>(1) << channel);
    }
    markRuntimeStateDirty();

    if (status_ != DeviceStatus::Ready) {
        return false;
    }

    I2cBusDevice::DependencyTransaction transaction;
    if (beginDependencyTransaction(transaction) != DependencyAccessResult::Ready || transaction.driver() == nullptr) {
        return false;
    }
    if (!writeCurrentStates(*transaction.driver())) {
        recordFailure(now);
        return false;
    }
    recordSuccess();
    return true;
}

uint32_t Pcf857xExpanderDeviceBase::channelStatesBitmask() const {
    return channelStates_;
}

const BusRuntimeDiagnostics& Pcf857xExpanderDeviceBase::diagnostics() const {
    return diagnostics_;
}

void Pcf857xExpanderDeviceBase::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}
bool Pcf857xExpanderDeviceBase::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = pcf857xExpanderConfigSize(config_);
    return encodeFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Pcf857xExpanderDeviceBase::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Pcf857xExpanderConfigV2 config{};
    if (!decodePcf857xExpanderConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    const bool changed = config.i2cAddress != config_.i2cAddress || config.inverted != config_.inverted;
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = changed;
    plan.resetStateMachine = changed;
    return plan;
}

bool Pcf857xExpanderDeviceBase::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Pcf857xExpanderConfigV2 config{};
    if (!decodePcf857xExpanderConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

DeviceValidationResult Pcf857xExpanderDeviceBase::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return validateI2cConfig<Pcf857xExpanderConfigV2>(record, configBlob, decodePcf857xExpanderConfig);
}

uint32_t Pcf857xExpanderDeviceBase::channelMask() const {
    return (static_cast<uint32_t>(1) << channelCountImpl()) - 1U;
}

bool Pcf857xExpanderDeviceBase::writeCurrentStates(II2cBusDriver& driver) const {
    const uint32_t prepared = (config_.inverted != 0U ? ~channelStates_ : channelStates_) & channelMask();
    driver.beginTransmission(config_.i2cAddress);
    if (!writeChannelStates(driver, prepared)) {
        return false;
    }
    return driver.endTransmission(true) == 0U;
}

void Pcf857xExpanderDeviceBase::recordSuccess() {
    if (diagnostics_.recordSuccess()) {
        markRuntimeStateDirty();
    }
    consecutiveErrors_ = 0;
}

void Pcf857xExpanderDeviceBase::recordFailure(uint32_t now) {
    if (diagnostics_.recordError(1U, now, kDiagnosticsDebounceMs)) {
        markRuntimeStateDirty();
    }
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + (consecutiveErrors_ >= kFaultErrorThreshold ? kFaultRetryBackoffMs : kRetryBackoffMs);
}

SM_STATE(Pcf857xExpanderDeviceBase::Idle) {
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

SM_STATE(Pcf857xExpanderDeviceBase::Starting) {
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    clearStartRequested();
    SM_GOTO(Syncing);
}

SM_STATE(Pcf857xExpanderDeviceBase::Syncing) {
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
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
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    if (!writeCurrentStates(*transaction.driver())) {
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    recordSuccess();
    lastDependencyGeneration_ = transaction.generation();
    nextSyncAt_ = uptime() + kSyncIntervalMs;
    SM_GOTO(Ready);
}

SM_STATE(Pcf857xExpanderDeviceBase::Ready) {
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), nextSyncAt_)) {
        return;
    }
    SM_GOTO(Syncing);
}

SM_STATE(Pcf857xExpanderDeviceBase::RetryBackoff) {
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
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

SM_STATE(Pcf857xExpanderDeviceBase::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBusReady() && (reconfigureRequested_ || startRequested_ || lastDependencyGeneration_ == 0U)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Pcf857xExpanderDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    clearStartRequested();
    lastDependencyGeneration_ = 0;
    consecutiveErrors_ = 0;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    SM_GOTO(Starting);
}

SM_STATE(Pcf857xExpanderDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_ && config_.enabled != 0U) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Pcf857xExpanderDeviceBase::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
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

SM_STATE(Pcf857xExpanderDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
