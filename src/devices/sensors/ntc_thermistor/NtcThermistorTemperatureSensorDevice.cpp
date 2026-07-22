#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"

#include "devices/core/ConfigCodec.h"

#include <cmath>
#include <cstdint>

namespace ewfm {

namespace {
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
    : PolledTemperatureSensorDeviceBase(), config_(config) {
    applyFilterConfig();
}

const NtcThermistorTemperatureSensorConfigV1& NtcThermistorTemperatureSensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& NtcThermistorTemperatureSensorDevice::baseConfig() const {
    return config_;
}

uint32_t NtcThermistorTemperatureSensorDevice::pollIntervalMs() const {
    return config_.pollMs;
}

const SensorFilterConfigV1& NtcThermistorTemperatureSensorDevice::filterConfig() const {
    return config_.filter;
}

bool NtcThermistorTemperatureSensorDevice::reportAlways() const {
    return config_.reportAlways != 0U;
}

uint16_t NtcThermistorTemperatureSensorDevice::reportDeltaCentiCelsius() const {
    return config_.reportDeltaCentiCelsius;
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
    applyFilterConfig();
    if (pollChanged) {
        reschedulePoll(now);
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

bool NtcThermistorTemperatureSensorDevice::sensorReady() const {
    return dependencyReady(DeviceRole::AnalogInput) && analogInput_ != nullptr;
}

bool NtcThermistorTemperatureSensorDevice::sampleReading(uint32_t now, int32_t& milliCelsius, const char*& invalidStatus) {
    (void)now;
    AnalogInputReading input{};
    if (analogInput_ == nullptr || !analogInput_->latestAnalogInputReading(input) || !input.valid) {
        invalidStatus = kOutputOutOfRange;
        return false;
    }

    double resistanceOhms = 0.0;
    if (!ntcDividerResistanceOhms(input.milliVolts, config_.seriesResistorOhms, config_.supplyMilliVolts, resistanceOhms)) {
        invalidStatus = kOutputOutOfRange;
        return false;
    }

    NtcFormulaMode mode{};
    (void)ntcFormulaModeFromByte(config_.formulaMode, mode);
    const int32_t rawMilliCelsius =
        mode == NtcFormulaMode::Beta
            ? ntcBetaMilliCelsius(resistanceOhms, config_.nominalResistanceOhms, config_.nominalTempCentiCelsius, config_.betaCoefficient)
            : ntcSteinhartHartMilliCelsius(resistanceOhms, config_.steinhartA, config_.steinhartB, config_.steinhartC);
    if (rawMilliCelsius == INT32_MIN) {
        invalidStatus = kOutputOutOfRange;
        return false;
    }

    milliCelsius = rawMilliCelsius;
    return true;
}

} // namespace ewfm
