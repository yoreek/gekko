#include "devices/sensors/htu21/Htu21SensorDevice.h"

#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/sensors/htu21/Htu21Protocol.h"

#include <cmath>
#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Htu21SensorDevice

namespace {
constexpr uint32_t kBusyRetryMs = 100;
constexpr uint32_t kRetryBackoffMs = 1000;
constexpr uint32_t kFaultRetryBackoffMs = 30000;
constexpr uint8_t kFaultErrorThreshold = 3;

const char* kOutputNotReady = "not_ready";
const char* kOutputDisabled = "disabled";
const char* kOutputDependencyUnavailable = "dependency_unavailable";
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

Htu21SensorDevice::Htu21SensorDevice(const Htu21SensorConfigV3& config)
    : Htu21SensorDeviceBase((PState)&Htu21SensorDevice::Idle), config_(config) {
    temperatureFilter_.configure(config_.temperatureFilter);
    humidityFilter_.configure(config_.humidityFilter);
}

Htu21SensorDevice::Htu21SensorDevice(const Htu21SensorConfigV2& config)
    : Htu21SensorDevice([&config]() {
          Htu21SensorConfigV3 migrated{};
          migrated.migrateFrom(config);
          return migrated;
      }()) {}

const Htu21SensorConfigV3& Htu21SensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Htu21SensorDevice::baseConfig() const {
    return config_;
}

const ITemperatureReadingRuntime* Htu21SensorDevice::temperatureReadingRuntime() const {
    return this;
}

const IHumidityReadingRuntime* Htu21SensorDevice::humidityReadingRuntime() const {
    return this;
}

bool Htu21SensorDevice::latestTemperatureReading(TemperatureReading& reading) const {
    reading = temperaturePublisher_.reading();
    return true;
}

const char* Htu21SensorDevice::latestTemperatureStatus() const {
    return temperaturePublisher_.status();
}

bool Htu21SensorDevice::latestHumidityReading(HumidityReading& reading) const {
    reading = humidityPublisher_.reading();
    return true;
}

const char* Htu21SensorDevice::latestHumidityStatus() const {
    return humidityPublisher_.status();
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
    temperatureFilter_.configure(config_.temperatureFilter);
    humidityFilter_.configure(config_.humidityFilter);
    if (pollChanged) {
        nextPollAt_ = now + config_.pollMs;
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

bool Htu21SensorDevice::triggerMeasurement(II2cBusDriver& driver, uint8_t command) const {
    driver.beginTransmission(config_.i2cAddress);
    driver.write(command);
    return driver.endTransmission(true) == 0U;
}

bool Htu21SensorDevice::readMeasurement(II2cBusDriver& driver, uint16_t& raw, const char*& errorStatus) const {
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
    raw = value;
    return true;
}

TemperatureUnit Htu21SensorDevice::outputUnit() const {
    TemperatureUnit unit{TemperatureUnit::Celsius};
    (void)temperatureUnitFromByte(config_.outputUnit, unit);
    return unit;
}

void Htu21SensorDevice::publishReadings(int32_t rawMilliCelsius, int32_t rawMilliPercent, uint32_t now) {
    temperaturePublisher_.configure(config_.reportAlways != 0U, config_.reportDeltaCentiCelsius);
    humidityPublisher_.configure(config_.reportAlways != 0U, config_.reportDeltaCentiPercent);

    const float filteredTemperature = temperatureFilter_.apply(static_cast<float>(rawMilliCelsius));
    float filteredHumidity = humidityFilter_.apply(static_cast<float>(rawMilliPercent));
    // Calibration can push the filtered value outside the physical range - clamp after filtering.
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

void Htu21SensorDevice::invalidateReadings(const char* status) {
    bool dirty = temperaturePublisher_.invalidate(status);
    dirty = humidityPublisher_.invalidate(status) || dirty;
    if (dirty) {
        markRuntimeStateDirty();
    }
}

void Htu21SensorDevice::recordFailure(uint32_t now) {
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + (consecutiveErrors_ >= kFaultErrorThreshold ? kFaultRetryBackoffMs : kRetryBackoffMs);
}

SM_STATE(Htu21SensorDevice::Idle) {
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

SM_STATE(Htu21SensorDevice::Starting) {
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

    if (!probeI2cPresence(*transaction.driver()) || !triggerMeasurement(*transaction.driver(), kHtu21CmdSoftReset)) {
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
    measureDeadline_ = uptime() + kHtu21SoftResetMs;
    SM_GOTO(ResetDelay);
}

SM_STATE(Htu21SensorDevice::ResetDelay) {
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
    if (!EWFM_SM_TIME_REACHED(uptime(), measureDeadline_)) {
        return;
    }
    SM_GOTO(TriggerTemperature);
}

SM_STATE(Htu21SensorDevice::TriggerTemperature) {
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

    if (!triggerMeasurement(*transaction.driver(), kHtu21CmdMeasureTemperatureNoHold)) {
        invalidateReadings(kOutputNotFound);
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    measureDeadline_ = uptime() + kHtu21TemperatureMeasureMs;
    SM_GOTO(ReadTemperature);
}

SM_STATE(Htu21SensorDevice::ReadTemperature) {
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

    uint16_t raw = 0;
    const char* errorStatus = kOutputNotFound;
    if (!readMeasurement(*transaction.driver(), raw, errorStatus)) {
        invalidateReadings(errorStatus);
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    pendingMilliCelsius_ = htu21RawToMilliCelsius(raw);
    SM_GOTO(TriggerHumidity);
}

SM_STATE(Htu21SensorDevice::TriggerHumidity) {
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

    if (!triggerMeasurement(*transaction.driver(), kHtu21CmdMeasureHumidityNoHold)) {
        invalidateReadings(kOutputNotFound);
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    measureDeadline_ = uptime() + kHtu21HumidityMeasureMs;
    SM_GOTO(ReadHumidity);
}

SM_STATE(Htu21SensorDevice::ReadHumidity) {
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

    uint16_t raw = 0;
    const char* errorStatus = kOutputNotFound;
    if (!readMeasurement(*transaction.driver(), raw, errorStatus)) {
        invalidateReadings(errorStatus);
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    publishReadings(pendingMilliCelsius_, htu21RawToMilliPercent(raw), uptime());
    consecutiveErrors_ = 0;
    nextPollAt_ = uptime() + config_.pollMs;
    SM_GOTO(Ready);
}

SM_STATE(Htu21SensorDevice::Ready) {
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
    SM_GOTO(TriggerTemperature);
}

SM_STATE(Htu21SensorDevice::RetryBackoff) {
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

SM_STATE(Htu21SensorDevice::DependencyBlocked) {
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

SM_STATE(Htu21SensorDevice::Reconfiguring) {
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

SM_STATE(Htu21SensorDevice::Disabled) {
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

SM_STATE(Htu21SensorDevice::Faulted) {
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

SM_STATE(Htu21SensorDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    invalidateReadings(kOutputNotReady);
    setDeleted();
}

} // namespace ewfm
