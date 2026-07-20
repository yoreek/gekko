#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDevice.h"

#include "devices/analog/adc/AdcAttenuationCodec.h"
#include "devices/analog/adc/ArduinoAdcInputDriver.h"
#include "devices/analog/input/AnalogInputRawCode.h"
#include "devices/switch/gpio/ArduinoGpioOutputDriver.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Cd74hc4067HubDevice

namespace {
const char* kOutputNotReady = "not_ready";
const char* kOutputInvalidChannel = "invalid_channel";
} // namespace

Cd74hc4067HubDevice::Cd74hc4067HubDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Cd74hc4067HubDevice([&configBlob]() {
          Cd74hc4067HubDeviceConfigV1 config{};
          (void)decodeValidatedFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Cd74hc4067HubDevice::Cd74hc4067HubDevice(const Cd74hc4067HubDeviceConfigV1& config)
    : Cd74hc4067HubDevice(config, defaultArduinoGpioOutputDriver(), defaultArduinoAdcInputDriver()) {}

Cd74hc4067HubDevice::Cd74hc4067HubDevice(const Cd74hc4067HubDeviceConfigV1& config, IGpioOutputDriver& gpioDriver,
                                         IAdcInputDriver& adcDriver)
    : DeviceRuntimeBase((PState)&Cd74hc4067HubDevice::Idle), config_(config), gpioDriver_(gpioDriver), adcDriver_(adcDriver) {}

const Cd74hc4067HubDeviceConfigV1& Cd74hc4067HubDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Cd74hc4067HubDevice::baseConfig() const {
    return config_;
}

const IAnalogInputHubRuntime* Cd74hc4067HubDevice::analogInputHubRuntime() const {
    return this;
}

bool Cd74hc4067HubDevice::hasDuplicateDependentChannel(uint8_t channel, const IDeviceRuntime* ignoreDependent) const {
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

uint8_t Cd74hc4067HubDevice::channelCount() const {
    return kCd74hc4067ChannelCount;
}

uint32_t Cd74hc4067HubDevice::generation() const {
    return generation_;
}

AnalogInputHubPollResult Cd74hc4067HubDevice::pollChannelReading(uint8_t channel, DeviceId requester, uint32_t now,
                                                                 AnalogInputReading& outReading, const char*& outStatus) {
    (void)now;
    if (status_ != DeviceStatus::Ready) {
        outStatus = kOutputNotReady;
        return AnalogInputHubPollResult::Fault;
    }
    if (channel >= kCd74hc4067ChannelCount) {
        outStatus = kOutputInvalidChannel;
        return AnalogInputHubPollResult::Fault;
    }
    if (ownerRequester_ != 0U && !(ownerChannel_ == channel && ownerRequester_ == requester)) {
        return AnalogInputHubPollResult::Busy;
    }

    ownerChannel_ = channel;
    ownerRequester_ = requester;

    if (muxSelectedChannel_ != channel) {
        for (uint8_t bit = 0; bit < 4U; ++bit) {
            (void)gpioDriver_.write(config_.selectPins[bit], ((channel >> bit) & 0x01U) != 0U);
        }
        muxSelectedChannel_ = channel;
        return AnalogInputHubPollResult::Pending;
    }

    const int32_t milliVolts = static_cast<int32_t>(adcDriver_.readMilliVolts(config_.sigPin));
    outReading.rawCode = analogInputRawCode(milliVolts);
    outReading.milliVolts = milliVolts;
    outReading.measuredAtMs = now;
    outReading.valid = true;
    outStatus = "ok";

    ownerChannel_ = kCd74hc4067UnusedPin;
    ownerRequester_ = 0U;
    return AnalogInputHubPollResult::Ready;
}

void Cd74hc4067HubDevice::releaseChannelRequest(uint8_t channel, DeviceId requester) {
    if (ownerChannel_ == channel && ownerRequester_ == requester) {
        ownerChannel_ = kCd74hc4067UnusedPin;
        ownerRequester_ = 0U;
    }
}

void Cd74hc4067HubDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool Cd74hc4067HubDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = cd74hc4067HubDeviceConfigSize(config_);
    return encodeFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Cd74hc4067HubDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Cd74hc4067HubDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    bool pinsChanged = config.enablePin != config_.enablePin || config.sigPin != config_.sigPin;
    for (uint8_t index = 0; index < 4U; ++index) {
        pinsChanged = pinsChanged || config.selectPins[index] != config_.selectPins[index];
    }

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = pinsChanged;
    plan.resetStateMachine = pinsChanged;
    return plan;
}

bool Cd74hc4067HubDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Cd74hc4067HubDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

void Cd74hc4067HubDevice::end(uint32_t now) {
    (void)now;
    releaseHardware();
}

bool Cd74hc4067HubDevice::configureHardware() {
    bool ok = true;
    for (uint8_t pin : config_.selectPins) {
        ok = gpioDriver_.configureOutput(pin, false) && ok;
    }
    if (config_.enablePin != kCd74hc4067UnusedPin) {
        ok = gpioDriver_.configureOutput(config_.enablePin, false) && ok; // active-low enable
    }
    AdcAttenuation attenuation{};
    if (!attenuationFromByte(config_.sigAttenuation, attenuation) || !adcDriver_.configurePin(config_.sigPin, attenuation)) {
        ok = false;
    }
    if (ok) {
        bumpGeneration();
    }
    muxSelectedChannel_ = kCd74hc4067UnusedPin;
    ownerChannel_ = kCd74hc4067UnusedPin;
    ownerRequester_ = 0U;
    return ok;
}

void Cd74hc4067HubDevice::releaseHardware() {
    for (uint8_t pin : config_.selectPins) {
        gpioDriver_.release(pin);
    }
    if (config_.enablePin != kCd74hc4067UnusedPin) {
        gpioDriver_.release(config_.enablePin);
    }
    adcDriver_.release(config_.sigPin);
    muxSelectedChannel_ = kCd74hc4067UnusedPin;
    ownerChannel_ = kCd74hc4067UnusedPin;
    ownerRequester_ = 0U;
}

void Cd74hc4067HubDevice::bumpGeneration() {
    ++generation_;
    if (generation_ == 0U) {
        ++generation_;
    }
}

DeviceTypeDescriptor Cd74hc4067HubDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kCd74hc4067HubTypeId;
    descriptor.name = "Cd74hc4067HubDevice";
    descriptor.currentConfigVersion = kCd74hc4067HubConfigVersion;
    descriptor.maxDependents = kCd74hc4067ChannelCount;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.providedRoles = ProvidedRoles::of({IAnalogInputHubRuntime::kProvidedRole});
    descriptor.createRuntime = &Cd74hc4067HubDevice::createRuntime;
    descriptor.validateConfig = &Cd74hc4067HubDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Cd74hc4067HubDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Cd74hc4067HubDevice(record, configBlob));
}

DeviceValidationResult Cd74hc4067HubDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "cd74hc4067 hub config exceeds supported size"};
    }
    Cd74hc4067HubDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "cd74hc4067 hub config is invalid"};
    }
    return {};
}

SM_STATE(Cd74hc4067HubDevice::Idle) {
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

SM_STATE(Cd74hc4067HubDevice::Starting) {
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
    if (!configureHardware()) {
        status_ = DeviceStatus::Faulted;
        requestFault();
        SM_GOTO(Faulted);
    }

    clearStartRequested();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(Cd74hc4067HubDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        releaseHardware();
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (faultRequested_) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
}

SM_STATE(Cd74hc4067HubDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    releaseHardware();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    markRuntimeStateDirty();
    SM_GOTO(Starting);
}

SM_STATE(Cd74hc4067HubDevice::Disabled) {
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

SM_STATE(Cd74hc4067HubDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    releaseHardware();
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
        clearFaultRequested();
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Cd74hc4067HubDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    releaseHardware();
    setDeleted();
}

} // namespace ewfm
