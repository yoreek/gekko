#include "devices/analog/input/ads1115/Ads1115HubDevice.h"

#include "devices/analog/input/AnalogInputRawCode.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Ads1115HubDevice

namespace {
constexpr uint32_t kBusyRetryMs = 100;
constexpr uint32_t kRetryBackoffMs = 1000;
constexpr uint32_t kFaultRetryBackoffMs = 30000;
constexpr uint8_t kFaultErrorThreshold = 3;

const char* kOutputNotReady = "not_ready";
const char* kOutputInvalidChannel = "invalid_channel";
const char* kOutputDependencyUnavailable = "dependency_unavailable";
const char* kOutputNotFound = "not_found";
} // namespace

Ads1115HubDevice::Ads1115HubDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Ads1115HubDevice([&configBlob]() {
          Ads1115HubDeviceConfigV1 config{};
          (void)decodeAds1115HubDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Ads1115HubDevice::Ads1115HubDevice(const Ads1115HubDeviceConfigV1& config)
    : Ads1115HubRuntimeBase((PState)&Ads1115HubDevice::Idle), config_(config) {}

const Ads1115HubDeviceConfigV1& Ads1115HubDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Ads1115HubDevice::baseConfig() const {
    return config_;
}

const IAnalogInputHubRuntime* Ads1115HubDevice::analogInputHubRuntime() const {
    return this;
}

bool Ads1115HubDevice::hasDuplicateDependentChannel(uint8_t channel, const IDeviceRuntime* ignoreDependent) const {
    for (const IDeviceRuntime* dependent : dependentRuntimes()) {
        if (dependent == nullptr || dependent == ignoreDependent) {
            continue;
        }
        uint8_t dependentChannel{0};
        if (!dependent->expanderChannel(dependentChannel)) {
            continue;
        }
        if (dependentChannel == channel) {
            return true;
        }
    }
    return false;
}

uint8_t Ads1115HubDevice::channelCount() const {
    return kAds1115ChannelCount;
}

uint32_t Ads1115HubDevice::generation() const {
    return ownGeneration_;
}

void Ads1115HubDevice::releaseOwnership() {
    ownerChannel_ = 0xFFU;
    ownerRequester_ = 0U;
    phase_ = ConversionPhase::Idle;
}

void Ads1115HubDevice::bumpGeneration() {
    ++ownGeneration_;
    if (ownGeneration_ == 0U) {
        ++ownGeneration_;
    }
}

void Ads1115HubDevice::recordFailure(uint32_t now) {
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + (consecutiveErrors_ >= kFaultErrorThreshold ? kFaultRetryBackoffMs : kRetryBackoffMs);
    if (consecutiveErrors_ >= kFaultErrorThreshold) {
        requestFault();
    }
}

AnalogInputHubPollResult Ads1115HubDevice::pollChannelReading(uint8_t channel, DeviceId requester, uint32_t now,
                                                              AnalogInputReading& outReading, const char*& outStatus) {
    if (status_ != DeviceStatus::Ready) {
        outStatus = kOutputNotReady;
        return AnalogInputHubPollResult::Fault;
    }
    if (channel >= kAds1115ChannelCount) {
        outStatus = kOutputInvalidChannel;
        return AnalogInputHubPollResult::Fault;
    }
    if (ownerRequester_ != 0U && !(ownerChannel_ == channel && ownerRequester_ == requester)) {
        return AnalogInputHubPollResult::Busy;
    }

    ownerChannel_ = channel;
    ownerRequester_ = requester;

    Ads1115Gain gain{};
    (void)ads1115GainFromByte(config_.gain, gain);
    Ads1115DataRate dataRate{};
    (void)ads1115DataRateFromByte(config_.dataRateSps, dataRate);

    if (phase_ == ConversionPhase::Idle) {
        I2cBusDevice::DependencyTransaction transaction;
        const DependencyAccessResult access = beginDependencyTransaction(transaction);
        if (access == DependencyAccessResult::Missing) {
            releaseOwnership();
            outStatus = kOutputDependencyUnavailable;
            return AnalogInputHubPollResult::Fault;
        }
        if (access == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
            return AnalogInputHubPollResult::Pending;
        }

        II2cBusDriver& driver = *transaction.driver();
        const uint16_t configValue = ads1115ConfigRegisterValue(channel, gain, dataRate);
        driver.beginTransmission(config_.i2cAddress);
        driver.write(kAds1115RegConfig);
        driver.write(static_cast<uint8_t>(configValue >> 8));
        driver.write(static_cast<uint8_t>(configValue & 0xFFU));
        if (driver.endTransmission(true) != 0U) {
            releaseOwnership();
            recordFailure(now);
            outStatus = kOutputNotFound;
            return AnalogInputHubPollResult::Fault;
        }

        phase_ = ConversionPhase::Converting;
        conversionDeadline_ = now + ads1115ConversionTimeMs(dataRate);
        return AnalogInputHubPollResult::Pending;
    }

    if (!EWFM_SM_TIME_REACHED(now, conversionDeadline_)) {
        return AnalogInputHubPollResult::Pending;
    }

    I2cBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult access = beginDependencyTransaction(transaction);
    if (access == DependencyAccessResult::Missing) {
        releaseOwnership();
        outStatus = kOutputDependencyUnavailable;
        return AnalogInputHubPollResult::Fault;
    }
    if (access == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        return AnalogInputHubPollResult::Pending;
    }

    II2cBusDriver& driver = *transaction.driver();
    driver.beginTransmission(config_.i2cAddress);
    driver.write(kAds1115RegConversion);
    if (driver.endTransmission(false) != 0U || driver.requestFrom(config_.i2cAddress, 2, true) != 2U) {
        releaseOwnership();
        recordFailure(now);
        outStatus = kOutputNotFound;
        return AnalogInputHubPollResult::Fault;
    }
    const uint8_t msb = static_cast<uint8_t>(driver.read());
    const uint8_t lsb = static_cast<uint8_t>(driver.read());
    const int16_t raw = static_cast<int16_t>(static_cast<uint16_t>((static_cast<uint16_t>(msb) << 8) | lsb));

    consecutiveErrors_ = 0;
    releaseOwnership();

    const int32_t milliVolts = ads1115RawToMilliVolts(raw, gain);
    outReading.rawCode = analogInputRawCode(milliVolts, ads1115GainFullScaleMilliVolts(gain));
    outReading.milliVolts = milliVolts;
    outReading.measuredAtMs = now;
    outReading.valid = true;
    outStatus = "ok";
    return AnalogInputHubPollResult::Ready;
}

void Ads1115HubDevice::releaseChannelRequest(uint8_t channel, DeviceId requester) {
    if (ownerChannel_ == channel && ownerRequester_ == requester) {
        releaseOwnership();
    }
}

void Ads1115HubDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool Ads1115HubDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ads1115HubDeviceConfigSize(config_);
    return encodeFixedConfigBlob(Ads1115HubDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Ads1115HubDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Ads1115HubDeviceConfigV1 config{};
    if (!decodeAds1115HubDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    const bool changed = config.i2cAddress != config_.i2cAddress;
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = changed;
    plan.resetStateMachine = changed;
    return plan;
}

bool Ads1115HubDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Ads1115HubDeviceConfigV1 config{};
    if (!decodeAds1115HubDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

DeviceTypeDescriptor Ads1115HubDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kAds1115HubTypeId;
    descriptor.name = "Ads1115HubDevice";
    descriptor.currentConfigVersion = kAds1115HubConfigVersion;
    descriptor.maxDependents = kAds1115ChannelCount;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::I2CBus, true}};
    descriptor.providedRoles = ProvidedRoles::of({IAnalogInputHubRuntime::kProvidedRole});
    descriptor.createRuntime = &Ads1115HubDevice::createRuntime;
    descriptor.validateConfig = &Ads1115HubDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Ads1115HubDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Ads1115HubDevice(record, configBlob));
}

DeviceValidationResult Ads1115HubDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return validateI2cConfig<Ads1115HubDeviceConfigV1>(record, configBlob, decodeAds1115HubDeviceConfig);
}

SM_STATE(Ads1115HubDevice::Idle) {
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

SM_STATE(Ads1115HubDevice::Starting) {
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

    I2cBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult access = beginDependencyTransaction(transaction);
    if (access == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (access == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }
    if (!probeI2cPresence(*transaction.driver())) {
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    lastDependencyGeneration_ = transaction.generation();
    clearStartRequested();
    consecutiveErrors_ = 0;
    releaseOwnership();
    bumpGeneration();
    status_ = DeviceStatus::Ready;
    markRuntimeStateDirty();
    SM_GOTO(Ready);
}

SM_STATE(Ads1115HubDevice::Ready) {
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
    if (faultRequested_) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
}

SM_STATE(Ads1115HubDevice::RetryBackoff) {
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

SM_STATE(Ads1115HubDevice::DependencyBlocked) {
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

SM_STATE(Ads1115HubDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    clearStartRequested();
    lastDependencyGeneration_ = 0;
    consecutiveErrors_ = 0;
    releaseOwnership();
    markRuntimeStateDirty();
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

SM_STATE(Ads1115HubDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    releaseOwnership();
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

SM_STATE(Ads1115HubDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    releaseOwnership();
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
        clearFaultRequested();
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), retryDeadline_)) {
        return;
    }
    SM_GOTO(Starting);
}

SM_STATE(Ads1115HubDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    releaseOwnership();
    setDeleted();
}

} // namespace ewfm
