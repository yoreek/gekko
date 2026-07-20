#include "devices/analog/input/port/AnalogPortInputDevice.h"

#include "devices/analog/adc/AdcAttenuationCodec.h"
#include "devices/analog/adc/ArduinoAdcInputDriver.h"
#include "devices/analog/input/AnalogInputRawCode.h"

#include <cstdint>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS AnalogPortInputDevice

namespace {
const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
const char* kOutputNotFound = "not_found";
} // namespace

AnalogPortInputDevice::AnalogPortInputDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : AnalogPortInputDevice([&configBlob]() {
          AnalogPortInputDeviceConfigV1 config{};
          (void)decodeValidatedFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

AnalogPortInputDevice::AnalogPortInputDevice(const AnalogPortInputDeviceConfigV1& config)
    : AnalogPortInputDevice(config, defaultArduinoAdcInputDriver()) {}

AnalogPortInputDevice::AnalogPortInputDevice(const AnalogPortInputDeviceConfigV1& config, IAdcInputDriver& driver)
    : DeviceRuntimeBase((PState)&AnalogPortInputDevice::Idle), config_(config), driver_(driver) {}

const AnalogPortInputDeviceConfigV1& AnalogPortInputDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& AnalogPortInputDevice::baseConfig() const {
    return config_;
}

const AnalogInputReading& AnalogPortInputDevice::reading() const {
    return publisher_.reading();
}

const char* AnalogPortInputDevice::outputStatus() const {
    return publisher_.status();
}

const IAnalogInputRuntime* AnalogPortInputDevice::analogInputRuntime() const {
    return this;
}

bool AnalogPortInputDevice::latestAnalogInputReading(AnalogInputReading& reading) const {
    reading = publisher_.reading();
    return true;
}

const char* AnalogPortInputDevice::latestAnalogInputStatus() const {
    return publisher_.status();
}

void AnalogPortInputDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool AnalogPortInputDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = analogPortInputDeviceConfigSize(config_);
    return encodeFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

bool AnalogPortInputDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    AnalogPortInputDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    const bool pollChanged = config.poll.pollMs != config_.poll.pollMs;
    config_ = config;
    if (pollChanged) {
        nextPollAt_ = now + config_.poll.pollMs;
    }
    return true;
}

void AnalogPortInputDevice::end(uint32_t now) {
    releaseHardware(now);
}

DeviceTypeDescriptor AnalogPortInputDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kAnalogPortInputTypeId;
    descriptor.name = "AnalogPortInputDevice";
    descriptor.currentConfigVersion = kAnalogPortInputConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.providedRoles = ProvidedRoles::of({IAnalogInputRuntime::kProvidedRole});
    descriptor.createRuntime = &AnalogPortInputDevice::createRuntime;
    descriptor.validateConfig = &AnalogPortInputDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> AnalogPortInputDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                     const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new AnalogPortInputDevice(record, configBlob));
}

DeviceValidationResult AnalogPortInputDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "analog port input config exceeds supported size"};
    }
    AnalogPortInputDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "analog port input config is invalid"};
    }
    return {};
}

bool AnalogPortInputDevice::configurePin() {
    AdcAttenuation attenuation{};
    if (!attenuationFromByte(config_.attenuation, attenuation)) {
        return false;
    }
    return driver_.configurePin(config_.gpioPin, attenuation);
}

void AnalogPortInputDevice::releaseHardware(uint32_t now) {
    (void)now;
    driver_.release(config_.gpioPin);
}

void AnalogPortInputDevice::performReading(uint32_t now) {
    const uint8_t samples = config_.poll.adcSamples == 0U ? 1U : config_.poll.adcSamples;
    accumulator_.reset(samples);
    for (uint8_t index = 0; index < samples; ++index) {
        accumulator_.add(static_cast<int32_t>(driver_.readMilliVolts(config_.gpioPin)));
    }
    const int32_t milliVolts = accumulator_.average();

    publisher_.configure(config_.poll.reportAlways != 0U, config_.poll.reportDeltaMilliVolts);
    if (publisher_.publish(analogInputRawCode(milliVolts), milliVolts, now)) {
        markRuntimeStateDirty();
    }
}

SM_STATE(AnalogPortInputDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
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

SM_STATE(AnalogPortInputDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        if (publisher_.invalidate(kOutputDisabled)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Disabled);
    }
    if (!configurePin()) {
        status_ = DeviceStatus::Faulted;
        if (publisher_.invalidate(kOutputNotFound)) {
            markRuntimeStateDirty();
        }
        requestFault();
        SM_GOTO(Faulted);
    }

    clearStartRequested();
    nextPollAt_ = uptime();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(AnalogPortInputDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        if (publisher_.invalidate(kOutputDisabled)) {
            markRuntimeStateDirty();
        }
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
    if (!EWFM_SM_TIME_REACHED(uptime(), nextPollAt_)) {
        return;
    }
    nextPollAt_ = uptime() + config_.poll.pollMs;
    performReading(uptime());
}

SM_STATE(AnalogPortInputDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    if (publisher_.invalidate(kOutputNotReady)) {
        markRuntimeStateDirty();
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    SM_GOTO(Starting);
}

SM_STATE(AnalogPortInputDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (publisher_.invalidate(kOutputDisabled)) {
        markRuntimeStateDirty();
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_ && config_.enabled != 0U) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(AnalogPortInputDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        if (publisher_.invalidate(kOutputNotReady)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        if (publisher_.invalidate(kOutputDisabled)) {
            markRuntimeStateDirty();
        }
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        clearFaultRequested();
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(AnalogPortInputDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    if (publisher_.invalidate(kOutputNotReady)) {
        markRuntimeStateDirty();
    }
    setDeleted();
}

} // namespace ewfm
