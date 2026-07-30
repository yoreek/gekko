#include "devices/sensors/dht11/Dht11SensorDevice.h"

#include "devices/core/ConfigCodec.h"
#include "devices/sensors/dht11/Dht11LineDriver.h"
#include "devices/sensors/dht11/Dht11Protocol.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Dht11SensorDevice

Dht11SensorDevice::Dht11SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Dht11SensorDevice(
          [&configBlob]() {
              Dht11SensorConfigV2 config{};
              (void)decodeDht11SensorConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          defaultArduinoDht11LineDriver()) {
    bindDeviceIdentity(record, configBlob);
}

Dht11SensorDevice::Dht11SensorDevice(const Dht11SensorConfigV2& config, IDht11LineDriver& lineDriver)
    : PolledTemperatureHumiditySensorDeviceBase(), config_(config), lineDriver_(lineDriver) {}

const Dht11SensorConfigV2& Dht11SensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Dht11SensorDevice::baseConfig() const {
    return config_;
}

void Dht11SensorDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool Dht11SensorDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = dht11SensorConfigSize(config_);
    return encodeFixedConfigBlob(Dht11SensorConfigV2::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Dht11SensorDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Dht11SensorConfigV2 config{};
    if (!decodeDht11SensorConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.gpioPin != config_.gpioPin || config.internalPullup != config_.internalPullup;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Dht11SensorDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    Dht11SensorConfigV2 config{};
    if (!decodeDht11SensorConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    const bool pollChanged = config.pollMs != config_.pollMs;
    config_ = config;
    if (pollChanged) {
        reschedulePoll(now);
    }
    return true;
}

DeviceTypeDescriptor Dht11SensorDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kDht11SensorTypeId;
    descriptor.name = "Dht11SensorDevice";
    descriptor.currentConfigVersion = kDht11SensorConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.providedRoles = ProvidedRoles::of({ITemperatureReadingRuntime::kProvidedRole});
    descriptor.createRuntime = &Dht11SensorDevice::createRuntime;
    descriptor.validateConfig = &Dht11SensorDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Dht11SensorDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Dht11SensorDevice(record, configBlob));
}

DeviceValidationResult Dht11SensorDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "dht11 config exceeds supported size"};
    }
    Dht11SensorConfigV2 config{};
    if (!decodeDht11SensorConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "dht11 config is invalid"};
    }
    return config.validate();
}

bool Dht11SensorDevice::sampleReading(uint32_t now, int32_t& milliCelsius, int32_t& milliPercent, const char*& invalidStatus) {
    (void)now;
    return dht11CaptureMeasurement(lineDriver_, config_.gpioPin, config_.internalPullup != 0U, milliCelsius, milliPercent, invalidStatus);
}

uint32_t Dht11SensorDevice::pollIntervalMs() const {
    return config_.pollMs;
}

const SensorFilterConfigV1& Dht11SensorDevice::temperatureFilterConfig() const {
    return config_.temperatureFilter;
}

const SensorFilterConfigV1& Dht11SensorDevice::humidityFilterConfig() const {
    return config_.humidityFilter;
}

bool Dht11SensorDevice::reportAlways() const {
    return config_.reportAlways != 0U;
}

uint16_t Dht11SensorDevice::reportDeltaCentiCelsius() const {
    return config_.reportDeltaCentiCelsius;
}

uint16_t Dht11SensorDevice::reportDeltaCentiPercent() const {
    return config_.reportDeltaCentiPercent;
}

uint32_t Dht11SensorDevice::startDelayMs() const {
    return kDht11DefaultPowerOnDelayMs;
}

} // namespace ewfm
