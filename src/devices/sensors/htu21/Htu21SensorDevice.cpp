#include "devices/sensors/htu21/Htu21SensorDevice.h"

#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/sensors/htu21/Htu21Protocol.h"

namespace ewfm {

namespace {
constexpr uint8_t kTemperaturePhase = 0U;
constexpr uint8_t kHumidityPhase = 1U;

const char* kOutputNotFound = "not_found";
const char* kOutputCrcError = "crc_error";
} // namespace

Htu21SensorDevice::Htu21SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Htu21SensorDevice([&configBlob]() {
          Htu21SensorConfigV3 config{};
          (void)decodeHtu21SensorConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Htu21SensorDevice::Htu21SensorDevice(const Htu21SensorConfigV3& config) : Htu21SensorDeviceBase(), config_(config) {
    applyFilterConfig();
}

const Htu21SensorConfigV3& Htu21SensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Htu21SensorDevice::baseConfig() const {
    return config_;
}

void Htu21SensorDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool Htu21SensorDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = htu21SensorConfigSize(config_);
    return encodeFixedConfigBlob(Htu21SensorConfigV3::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Htu21SensorDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Htu21SensorConfigV3 config{};
    if (!decodeHtu21SensorConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.i2cAddress != config_.i2cAddress;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Htu21SensorDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    Htu21SensorConfigV3 config{};
    if (!decodeHtu21SensorConfig(configBlob.data(), configBlob.size(), config)) {
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

DeviceTypeDescriptor Htu21SensorDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kHtu21SensorTypeId;
    descriptor.name = "Htu21SensorDevice";
    descriptor.currentConfigVersion = kHtu21SensorConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::I2CBus, true}};
    descriptor.providedRoles = ProvidedRoles::of({ITemperatureReadingRuntime::kProvidedRole});
    descriptor.createRuntime = &Htu21SensorDevice::createRuntime;
    descriptor.validateConfig = &Htu21SensorDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Htu21SensorDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Htu21SensorDevice(record, configBlob));
}

DeviceValidationResult Htu21SensorDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return validateI2cConfig<Htu21SensorConfigV3>(record, configBlob, decodeHtu21SensorConfig);
}

bool Htu21SensorDevice::sendInitCommand(II2cBusDriver& driver) const {
    if (!probeI2cPresence(driver)) {
        return false;
    }
    driver.beginTransmission(config_.i2cAddress);
    driver.write(kHtu21CmdSoftReset);
    return driver.endTransmission(true) == 0U;
}

uint32_t Htu21SensorDevice::initDelayMs() const {
    return kHtu21SoftResetMs;
}

uint8_t Htu21SensorDevice::measurementPhaseCount() const {
    return 2U;
}

bool Htu21SensorDevice::sendPhaseTrigger(II2cBusDriver& driver, uint8_t phaseIndex) const {
    const uint8_t command = (phaseIndex == kTemperaturePhase) ? kHtu21CmdMeasureTemperatureNoHold : kHtu21CmdMeasureHumidityNoHold;
    driver.beginTransmission(config_.i2cAddress);
    driver.write(command);
    return driver.endTransmission(true) == 0U;
}

uint32_t Htu21SensorDevice::phaseDelayMs(uint8_t phaseIndex) const {
    return (phaseIndex == kTemperaturePhase) ? kHtu21TemperatureMeasureMs : kHtu21HumidityMeasureMs;
}

bool Htu21SensorDevice::readPhase(II2cBusDriver& driver, uint8_t phaseIndex, const char*& errorStatus) {
    if (driver.requestFrom(config_.i2cAddress, kHtu21MeasureResponseBytes, true) != kHtu21MeasureResponseBytes) {
        errorStatus = kOutputNotFound;
        return false;
    }
    const uint8_t msb = static_cast<uint8_t>(driver.read());
    const uint8_t lsb = static_cast<uint8_t>(driver.read());
    const uint8_t crc = static_cast<uint8_t>(driver.read());
    const uint16_t value = static_cast<uint16_t>((static_cast<uint16_t>(msb) << 8) | lsb);
    if (!htu21CrcValid(value, crc)) {
        errorStatus = kOutputCrcError;
        return false;
    }

    if (phaseIndex == kTemperaturePhase) {
        pendingMilliCelsius_ = htu21RawToMilliCelsius(value);
    } else {
        pendingMilliPercent_ = htu21RawToMilliPercent(value);
    }
    return true;
}

void Htu21SensorDevice::computeReadings(int32_t& milliCelsius, int32_t& milliPercent) const {
    milliCelsius = pendingMilliCelsius_;
    milliPercent = pendingMilliPercent_;
}

uint32_t Htu21SensorDevice::pollIntervalMs() const {
    return config_.pollMs;
}

const SensorFilterConfigV1& Htu21SensorDevice::temperatureFilterConfig() const {
    return config_.temperatureFilter;
}

const SensorFilterConfigV1& Htu21SensorDevice::humidityFilterConfig() const {
    return config_.humidityFilter;
}

bool Htu21SensorDevice::reportAlways() const {
    return config_.reportAlways != 0U;
}

uint16_t Htu21SensorDevice::reportDeltaCentiCelsius() const {
    return config_.reportDeltaCentiCelsius;
}

uint16_t Htu21SensorDevice::reportDeltaCentiPercent() const {
    return config_.reportDeltaCentiPercent;
}

TemperatureUnit Htu21SensorDevice::outputUnit() const {
    TemperatureUnit unit{TemperatureUnit::Celsius};
    (void)temperatureUnitFromByte(config_.outputUnit, unit);
    return unit;
}

} // namespace ewfm
