#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"

#include <cmath>
#include <cstdint>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS NtcThermistorTemperatureSensorDevice

namespace {
const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
const char* kOutputDependencyUnavailable = "dependency_unavailable";
const char* kOutputOutOfRange = "out_of_range";
} // namespace

NtcThermistorTemperatureSensorDevice::NtcThermistorTemperatureSensorDevice(const DeviceRegistryEntry& record,
                                                                           const DeviceConfigBlob& configBlob)
    : NtcThermistorTemperatureSensorDevice([&configBlob]() {
          NtcThermistorTemperatureSensorConfigV1 config{};
          (void)decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic, configBlob.data(), configBlob.size(),
                                               config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

NtcThermistorTemperatureSensorDevice::NtcThermistorTemperatureSensorDevice(const NtcThermistorTemperatureSensorConfigV1& config)
    : DeviceRuntimeBase((PState)&NtcThermistorTemperatureSensorDevice::Idle), config_(config) {
    filter_.configure(config_.filter);
}

const NtcThermistorTemperatureSensorConfigV1& NtcThermistorTemperatureSensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& NtcThermistorTemperatureSensorDevice::baseConfig() const {
    return config_;
}

const TemperatureReading& NtcThermistorTemperatureSensorDevice::reading() const {
    return publisher_.reading();
}

const char* NtcThermistorTemperatureSensorDevice::outputStatus() const {
    return publisher_.status();
}

const ITemperatureReadingRuntime* NtcThermistorTemperatureSensorDevice::temperatureReadingRuntime() const {
    return this;
}

bool NtcThermistorTemperatureSensorDevice::latestTemperatureReading(TemperatureReading& reading) const {
    reading = publisher_.reading();
    return true;
}

const char* NtcThermistorTemperatureSensorDevice::latestTemperatureStatus() const {
    return publisher_.status();
}

void NtcThermistorTemperatureSensorDevice::setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntime(role, dependencyRuntime);
    refreshCapabilityCache();
}

void NtcThermistorTemperatureSensorDevice::setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntimeAt(index, dependencyRuntime);
    refreshCapabilityCache();
}

void NtcThermistorTemperatureSensorDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
    refreshCapabilityCache();
}

bool NtcThermistorTemperatureSensorDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ntcThermistorTemperatureSensorConfigSize(config_);
    return encodeFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

bool NtcThermistorTemperatureSensorDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    NtcThermistorTemperatureSensorConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    const bool pollChanged = config.pollMs != config_.pollMs;
    config_ = config;
    filter_.configure(config_.filter);
    if (pollChanged) {
        nextPollAt_ = now + config_.pollMs;
    }
    return true;
}

DeviceTypeDescriptor NtcThermistorTemperatureSensorDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kNtcThermistorTemperatureSensorTypeId;
    descriptor.name = "NtcThermistorTemperatureSensorDevice";
    descriptor.currentConfigVersion = kNtcThermistorTemperatureSensorConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks1s = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::AnalogInput, true}};
    descriptor.providedRoles = ProvidedRoles::of({ITemperatureReadingRuntime::kProvidedRole});
    descriptor.createRuntime = &NtcThermistorTemperatureSensorDevice::createRuntime;
    descriptor.validateConfig = &NtcThermistorTemperatureSensorDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> NtcThermistorTemperatureSensorDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                                    const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new NtcThermistorTemperatureSensorDevice(record, configBlob));
}

DeviceValidationResult NtcThermistorTemperatureSensorDevice::validateConfig(const DeviceRegistryEntry& record,
                                                                            const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::AnalogInput) == 0U) {
        return {DeviceError::InvalidRelationship, "ntc thermistor requires an analog input dependency"};
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "ntc thermistor config exceeds supported size"};
    }
    NtcThermistorTemperatureSensorConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "ntc thermistor config is invalid"};
    }
    return {};
}

void NtcThermistorTemperatureSensorDevice::refreshCapabilityCache() {
    analogInput_ = nullptr;
    if (const IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::AnalogInput); dependency != nullptr) {
        analogInput_ = dependency->analogInputRuntime();
    }
}

bool NtcThermistorTemperatureSensorDevice::dependencyAnalogInputReady() const {
    return dependencyReady(DeviceRole::AnalogInput) && analogInput_ != nullptr;
}

TemperatureUnit NtcThermistorTemperatureSensorDevice::outputUnit() const {
    TemperatureUnit unit{TemperatureUnit::Celsius};
    (void)temperatureUnitFromByte(config_.outputUnit, unit);
    return unit;
}

void NtcThermistorTemperatureSensorDevice::performReading(uint32_t now) {
    AnalogInputReading input{};
    if (analogInput_ == nullptr || !analogInput_->latestAnalogInputReading(input) || !input.valid) {
        if (publisher_.invalidate(kOutputOutOfRange)) {
            markRuntimeStateDirty();
        }
        return;
    }

    double resistanceOhms = 0.0;
    if (!ntcDividerResistanceOhms(input.milliVolts, config_.seriesResistorOhms, config_.supplyMilliVolts, resistanceOhms)) {
        if (publisher_.invalidate(kOutputOutOfRange)) {
            markRuntimeStateDirty();
        }
        return;
    }

    NtcFormulaMode mode{};
    (void)ntcFormulaModeFromByte(config_.formulaMode, mode);
    const int32_t rawMilliCelsius =
        mode == NtcFormulaMode::Beta
            ? ntcBetaMilliCelsius(resistanceOhms, config_.nominalResistanceOhms, config_.nominalTempCentiCelsius, config_.betaCoefficient)
            : ntcSteinhartHartMilliCelsius(resistanceOhms, config_.steinhartA, config_.steinhartB, config_.steinhartC);
    if (rawMilliCelsius == INT32_MIN) {
        if (publisher_.invalidate(kOutputOutOfRange)) {
            markRuntimeStateDirty();
        }
        return;
    }

    const float filtered = filter_.apply(static_cast<float>(rawMilliCelsius));
    publisher_.configure(config_.reportAlways != 0U, config_.reportDeltaCentiCelsius);
    if (publisher_.publish(static_cast<int32_t>(std::lround(filtered)), now)) {
        markRuntimeStateDirty();
    }
}

SM_STATE(NtcThermistorTemperatureSensorDevice::Idle) {
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

SM_STATE(NtcThermistorTemperatureSensorDevice::Starting) {
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
    if (!dependencyAnalogInputReady()) {
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

SM_STATE(NtcThermistorTemperatureSensorDevice::Ready) {
    status_ = DeviceStatus::Ready;
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
    if (!dependencyAnalogInputReady()) {
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
    nextPollAt_ = uptime() + config_.pollMs;
    performReading(uptime());
}

SM_STATE(NtcThermistorTemperatureSensorDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
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
    if (dependencyAnalogInputReady() && (reconfigureRequested_ || startRequested_)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(NtcThermistorTemperatureSensorDevice::Reconfiguring) {
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
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    SM_GOTO(Starting);
}

SM_STATE(NtcThermistorTemperatureSensorDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (publisher_.invalidate(kOutputDisabled)) {
        markRuntimeStateDirty();
    }
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

SM_STATE(NtcThermistorTemperatureSensorDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    if (publisher_.invalidate(kOutputNotReady)) {
        markRuntimeStateDirty();
    }
    setDeleted();
}

} // namespace ewfm
