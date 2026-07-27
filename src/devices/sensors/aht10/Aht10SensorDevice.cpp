#include "devices/sensors/aht10/Aht10SensorDevice.h"

#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/sensors/aht10/Aht10Protocol.h"

namespace ewfm {

namespace {
const char* kOutputNotFound = "not_found";
} // namespace

Aht10SensorDevice::Aht10SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Aht10SensorDevice([&configBlob]() {
          Aht10SensorConfigV1 config{};
          (void)decodeAht10SensorConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Aht10SensorDevice::Aht10SensorDevice(const Aht10SensorConfigV1& config) : Aht10SensorDeviceBase(), config_(config) {
    applyFilterConfig();
}

const Aht10SensorConfigV1& Aht10SensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Aht10SensorDevice::baseConfig() const {
    return config_;
}

void Aht10SensorDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool Aht10SensorDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = aht10SensorConfigSize(config_);
    return encodeFixedConfigBlob(Aht10SensorConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Aht10SensorDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Aht10SensorConfigV1 config{};
    if (!decodeAht10SensorConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.i2cAddress != config_.i2cAddress;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Aht10SensorDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    Aht10SensorConfigV1 config{};
    if (!decodeAht10SensorConfig(configBlob.data(), configBlob.size(), config)) {
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

DeviceTypeDescriptor Aht10SensorDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kAht10SensorTypeId;
    descriptor.name = "Aht10SensorDevice";
    descriptor.currentConfigVersion = kAht10SensorConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::I2CBus, true}};
    descriptor.providedRoles = ProvidedRoles::of({ITemperatureReadingRuntime::kProvidedRole});
    descriptor.createRuntime = &Aht10SensorDevice::createRuntime;
    descriptor.validateConfig = &Aht10SensorDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Aht10SensorDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Aht10SensorDevice(record, configBlob));
}

DeviceValidationResult Aht10SensorDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return validateI2cConfig<Aht10SensorConfigV1>(record, configBlob, decodeAht10SensorConfig);
}

bool Aht10SensorDevice::sendInitCommand(II2cBusDriver& driver) const {
    if (!probeI2cPresence(driver)) {
        return false;
    }
    driver.beginTransmission(config_.i2cAddress);
    driver.write(kAht10CmdInit);
    driver.write(kAht10CmdInitArg1);
    driver.write(kAht10CmdInitArg2);
    return driver.endTransmission(true) == 0U;
}

uint32_t Aht10SensorDevice::initDelayMs() const {
    return kAht10InitMs;
}

uint8_t Aht10SensorDevice::measurementPhaseCount() const {
    return 1U;
}

bool Aht10SensorDevice::sendPhaseTrigger(II2cBusDriver& driver, uint8_t /*phaseIndex*/) const {
    driver.beginTransmission(config_.i2cAddress);
    driver.write(kAht10CmdMeasure);
    driver.write(kAht10CmdMeasureArg1);
    driver.write(kAht10CmdMeasureArg2);
    return driver.endTransmission(true) == 0U;
}

uint32_t Aht10SensorDevice::phaseDelayMs(uint8_t /*phaseIndex*/) const {
    return kAht10MeasureMs;
}

bool Aht10SensorDevice::readPhase(II2cBusDriver& driver, uint8_t /*phaseIndex*/, const char*& errorStatus) {
    uint8_t frame[kAht10MeasureResponseBytes]{};
    if (driver.requestFrom(config_.i2cAddress, kAht10MeasureResponseBytes, true) != kAht10MeasureResponseBytes) {
        errorStatus = kOutputNotFound;
        return false;
    }
    for (size_t index = 0; index < kAht10MeasureResponseBytes; ++index) {
        const int value = driver.read();
        if (value < 0) {
            errorStatus = kOutputNotFound;
            return false;
        }
        frame[index] = static_cast<uint8_t>(value);
    }
    return aht10DecodeMeasurement(frame, humidityRaw_, temperatureRaw_, errorStatus);
}

void Aht10SensorDevice::computeReadings(int32_t& milliCelsius, int32_t& milliPercent) const {
    milliCelsius = aht10RawToMilliCelsius(temperatureRaw_);
    milliPercent = aht10RawToMilliPercent(humidityRaw_);
}

uint32_t Aht10SensorDevice::pollIntervalMs() const {
    return config_.pollMs;
}

const SensorFilterConfigV1& Aht10SensorDevice::temperatureFilterConfig() const {
    return config_.temperatureFilter;
}

const SensorFilterConfigV1& Aht10SensorDevice::humidityFilterConfig() const {
    return config_.humidityFilter;
}

bool Aht10SensorDevice::reportAlways() const {
    return config_.reportAlways != 0U;
}

uint16_t Aht10SensorDevice::reportDeltaCentiCelsius() const {
    return config_.reportDeltaCentiCelsius;
}

uint16_t Aht10SensorDevice::reportDeltaCentiPercent() const {
    return config_.reportDeltaCentiPercent;
}

TemperatureUnit Aht10SensorDevice::outputUnit() const {
    TemperatureUnit unit{TemperatureUnit::Celsius};
    (void)temperatureUnitFromByte(config_.outputUnit, unit);
    return unit;
}

} // namespace ewfm
