#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"

#include "devices/sensors/ntc_thermistor/ArduinoAdcInputDriver.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS NtcThermistorTemperatureSensorDevice

namespace {
constexpr double kSupplyMilliVolts = 3300.0;
constexpr double kKelvinOffset = 273.15;

const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
const char* kOutputNotFound = "not_found";
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
    : NtcThermistorTemperatureSensorDevice(config, defaultArduinoAdcInputDriver()) {}

NtcThermistorTemperatureSensorDevice::NtcThermistorTemperatureSensorDevice(const NtcThermistorTemperatureSensorConfigV1& config,
                                                                           IAdcInputDriver& driver)
    : DeviceRuntimeBase((PState)&NtcThermistorTemperatureSensorDevice::Idle), config_(config), driver_(driver) {
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

void NtcThermistorTemperatureSensorDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool NtcThermistorTemperatureSensorDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ntcThermistorTemperatureSensorConfigSize(config_);
    return encodeFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan NtcThermistorTemperatureSensorDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    NtcThermistorTemperatureSensorConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool pinChanged = config.gpioPin != config_.gpioPin;
    const bool attenuationChanged = config.attenuation != config_.attenuation;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = pinChanged || attenuationChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
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

void NtcThermistorTemperatureSensorDevice::end(uint32_t now) {
    releaseHardware(now);
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
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "ntc thermistor config exceeds supported size"};
    }
    NtcThermistorTemperatureSensorConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "ntc thermistor config is invalid"};
    }
    return {};
}

bool NtcThermistorTemperatureSensorDevice::configurePin() {
    AdcAttenuation attenuation{};
    if (!attenuationFromByte(config_.attenuation, attenuation)) {
        return false;
    }
    return driver_.configurePin(config_.gpioPin, attenuation);
}

void NtcThermistorTemperatureSensorDevice::releaseHardware(uint32_t now) {
    (void)now;
    driver_.release(config_.gpioPin);
}

TemperatureUnit NtcThermistorTemperatureSensorDevice::outputUnit() const {
    TemperatureUnit unit{TemperatureUnit::Celsius};
    (void)temperatureUnitFromByte(config_.outputUnit, unit);
    return unit;
}

int32_t NtcThermistorTemperatureSensorDevice::readCentiCelsius() const {
    // Voltage divider: series resistor between Vcc and the ADC node, NTC between the ADC node
    // and GND. As the NTC heats up its resistance drops, so the measured node voltage drops too.
    // Rntc = seriesResistorOhms * Vout / (Vcc - Vout)
    const uint8_t samples = config_.adcSamples == 0U ? 1U : config_.adcSamples;
    uint64_t sum = 0;
    for (uint8_t index = 0; index < samples; ++index) {
        sum += driver_.readMilliVolts(config_.gpioPin);
    }
    const double vOutMv = static_cast<double>(sum) / static_cast<double>(samples);
    if (vOutMv <= 0.0 || vOutMv >= kSupplyMilliVolts) {
        return INT32_MIN;
    }

    const double rNtc = static_cast<double>(config_.seriesResistorOhms) * vOutMv / (kSupplyMilliVolts - vOutMv);
    const double nominalTempKelvin = static_cast<double>(config_.nominalTempCentiCelsius) / 100.0 + kKelvinOffset;
    const double inverseT = 1.0 / nominalTempKelvin + (1.0 / static_cast<double>(config_.betaCoefficient)) *
                                                          std::log(rNtc / static_cast<double>(config_.nominalResistanceOhms));
    if (inverseT <= 0.0) {
        return INT32_MIN;
    }

    const double tempCelsius = 1.0 / inverseT - kKelvinOffset;
    return static_cast<int32_t>(std::lround(tempCelsius * 1000.0));
}

void NtcThermistorTemperatureSensorDevice::performReading(uint32_t now) {
    const int32_t rawMilliCelsius = readCentiCelsius();
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
    if (!configurePin()) {
        status_ = DeviceStatus::Faulted;
        if (publisher_.invalidate(kOutputNotFound)) {
            markRuntimeStateDirty();
        }
        requestFault();
        SM_GOTO(Faulted);
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
    nextPollAt_ = uptime() + config_.pollMs;
    performReading(uptime());
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
        releaseHardware(uptime());
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
        releaseHardware(uptime());
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_ && config_.enabled != 0U) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(NtcThermistorTemperatureSensorDevice::Faulted) {
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

SM_STATE(NtcThermistorTemperatureSensorDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    if (publisher_.invalidate(kOutputNotReady)) {
        markRuntimeStateDirty();
    }
    setDeleted();
}

} // namespace ewfm
