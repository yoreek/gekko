#include "devices/sensors/aht10/Aht10SensorDevice.h"

#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/sensors/aht10/Aht10Protocol.h"

#include <cmath>
#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Aht10SensorDevice

namespace {
constexpr uint32_t kBusyRetryMs = 100;
constexpr uint32_t kRetryBackoffMs = 1000;
constexpr uint32_t kFaultRetryBackoffMs = 30000;
constexpr uint8_t kFaultErrorThreshold = 3;

const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
const char* kOutputDependencyUnavailable = "dependency_unavailable";
const char* kOutputNotFound = "not_found";
const char* kOutputBusy = "busy";
} // namespace

Aht10SensorDevice::Aht10SensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Aht10SensorDevice([&configBlob]() {
          Aht10SensorConfigV1 config{};
          (void)decodeAht10SensorConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Aht10SensorDevice::Aht10SensorDevice(const Aht10SensorConfigV1& config)
    : Aht10SensorDeviceBase((PState)&Aht10SensorDevice::Idle), config_(config) {
    temperatureFilter_.configure(config_.temperatureFilter);
    humidityFilter_.configure(config_.humidityFilter);
}

const Aht10SensorConfigV1& Aht10SensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Aht10SensorDevice::baseConfig() const {
    return config_;
}

const ITemperatureReadingRuntime* Aht10SensorDevice::temperatureReadingRuntime() const {
    return this;
}

const IHumidityReadingRuntime* Aht10SensorDevice::humidityReadingRuntime() const {
    return this;
}

bool Aht10SensorDevice::latestTemperatureReading(TemperatureReading& reading) const {
    reading = temperaturePublisher_.reading();
    return true;
}

const char* Aht10SensorDevice::latestTemperatureStatus() const {
    return temperaturePublisher_.status();
}

bool Aht10SensorDevice::latestHumidityReading(HumidityReading& reading) const {
    reading = humidityPublisher_.reading();
    return true;
}

const char* Aht10SensorDevice::latestHumidityStatus() const {
    return humidityPublisher_.status();
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
    temperatureFilter_.configure(config_.temperatureFilter);
    humidityFilter_.configure(config_.humidityFilter);
    if (pollChanged) {
        nextPollAt_ = now + config_.pollMs;
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

bool Aht10SensorDevice::sendCommand(II2cBusDriver& driver, uint8_t command, uint8_t arg1, uint8_t arg2) const {
    driver.beginTransmission(config_.i2cAddress);
    driver.write(command);
    driver.write(arg1);
    driver.write(arg2);
    return driver.endTransmission(true) == 0U;
}

bool Aht10SensorDevice::readMeasurement(II2cBusDriver& driver, uint32_t& humidityRaw, uint32_t& temperatureRaw,
                                        const char*& errorStatus) const {
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
    return aht10DecodeMeasurement(frame, humidityRaw, temperatureRaw, errorStatus);
}

TemperatureUnit Aht10SensorDevice::outputUnit() const {
    TemperatureUnit unit{TemperatureUnit::Celsius};
    (void)temperatureUnitFromByte(config_.outputUnit, unit);
    return unit;
}

void Aht10SensorDevice::publishReadings(int32_t rawMilliCelsius, int32_t rawMilliPercent, uint32_t now) {
    temperaturePublisher_.configure(config_.reportAlways != 0U, config_.reportDeltaCentiCelsius);
    humidityPublisher_.configure(config_.reportAlways != 0U, config_.reportDeltaCentiPercent);

    const float filteredTemperature = temperatureFilter_.apply(static_cast<float>(rawMilliCelsius));
    float filteredHumidity = humidityFilter_.apply(static_cast<float>(rawMilliPercent));
    if (filteredHumidity < 0.0F) {
        filteredHumidity = 0.0F;
    } else if (filteredHumidity > 100000.0F) {
        filteredHumidity = 100000.0F;
    }

    bool dirty = temperaturePublisher_.publish(static_cast<int32_t>(std::lround(filteredTemperature)), now);
    dirty = humidityPublisher_.publish(static_cast<int32_t>(std::lround(filteredHumidity)), now) || dirty;
    if (dirty) {
        markRuntimeStateDirty();
    }
}

void Aht10SensorDevice::invalidateReadings(const char* status) {
    bool dirty = temperaturePublisher_.invalidate(status);
    dirty = humidityPublisher_.invalidate(status) || dirty;
    if (dirty) {
        markRuntimeStateDirty();
    }
}

void Aht10SensorDevice::recordFailure(uint32_t now) {
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + (consecutiveErrors_ >= kFaultErrorThreshold ? kFaultRetryBackoffMs : kRetryBackoffMs);
}

SM_STATE(Aht10SensorDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Starting);
    }
}

SM_STATE(Aht10SensorDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }

    I2cBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    if (!probeI2cPresence(*transaction.driver()) ||
        !sendCommand(*transaction.driver(), kAht10CmdInit, kAht10CmdInitArg1, kAht10CmdInitArg2)) {
        invalidateReadings(kOutputNotFound);
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    lastDependencyGeneration_ = transaction.generation();
    clearStartRequested();
    temperatureFilter_.reset();
    humidityFilter_.reset();
    initDeadline_ = uptime() + kAht10InitMs;
    SM_GOTO(InitDelay);
}

SM_STATE(Aht10SensorDevice::InitDelay) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), initDeadline_)) {
        return;
    }
    SM_GOTO(TriggerMeasurement);
}

SM_STATE(Aht10SensorDevice::TriggerMeasurement) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
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
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    if (!sendCommand(*transaction.driver(), kAht10CmdMeasure, kAht10CmdMeasureArg1, kAht10CmdMeasureArg2)) {
        invalidateReadings(kOutputNotFound);
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    measureDeadline_ = uptime() + kAht10MeasureMs;
    SM_GOTO(ReadMeasurement);
}

SM_STATE(Aht10SensorDevice::ReadMeasurement) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), measureDeadline_)) {
        return;
    }

    I2cBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    uint32_t humidityRaw = 0;
    uint32_t temperatureRaw = 0;
    const char* errorStatus = kOutputNotFound;
    if (!readMeasurement(*transaction.driver(), humidityRaw, temperatureRaw, errorStatus)) {
        invalidateReadings(errorStatus);
        if (errorStatus != nullptr && std::strcmp(errorStatus, kOutputBusy) != 0) {
            recordFailure(uptime());
            if (consecutiveErrors_ >= kFaultErrorThreshold) {
                SM_GOTO(Faulted);
            }
        } else {
            retryDeadline_ = uptime() + kBusyRetryMs;
        }
        SM_GOTO(RetryBackoff);
    }

    publishReadings(aht10RawToMilliCelsius(temperatureRaw), aht10RawToMilliPercent(humidityRaw), uptime());
    consecutiveErrors_ = 0;
    nextPollAt_ = uptime() + config_.pollMs;
    SM_GOTO(Ready);
}

SM_STATE(Aht10SensorDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), nextPollAt_)) {
        return;
    }
    SM_GOTO(TriggerMeasurement);
}

SM_STATE(Aht10SensorDevice::RetryBackoff) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
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

SM_STATE(Aht10SensorDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (dependencyBusReady() && (reconfigureRequested_ || startRequested_ || lastDependencyGeneration_ == 0U)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Aht10SensorDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    clearStartRequested();
    lastDependencyGeneration_ = 0;
    consecutiveErrors_ = 0;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    SM_GOTO(Starting);
}

SM_STATE(Aht10SensorDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    invalidateReadings(kOutputDisabled);
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

SM_STATE(Aht10SensorDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReadings(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReadings(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReadings(kOutputDependencyUnavailable);
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

SM_STATE(Aht10SensorDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    invalidateReadings(kOutputNotReady);
    setDeleted();
}

} // namespace ewfm
