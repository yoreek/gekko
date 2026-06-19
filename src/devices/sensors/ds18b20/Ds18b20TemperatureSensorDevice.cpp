#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"

#include "debug/Debug.h"
#include "devices/sensors/ds18b20/Ds18b20OneWireProtocol.h"

#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Ds18b20TemperatureSensorDevice

namespace {
constexpr uint32_t kPowerUpDelayMs = 20;
constexpr uint32_t kRetryBackoffMs = 1000;
constexpr uint8_t kFaultErrorThreshold = 3;

const char* kOutputOk = "ok";
const char* kOutputNotReady = "not_ready";
const char* kOutputParentUnavailable = "parent_unavailable";
const char* kOutputParentBusy = "parent_busy";
const char* kOutputNotFound = "not_found";
const char* kOutputCrcError = "crc_error";
const char* kOutputOutOfRange = "out_of_range";
const char* kOutputDisabled = "disabled";
} // namespace

Ds18b20TemperatureSensorDevice::Ds18b20TemperatureSensorDevice(const DeviceRecord& record)
    : Ds18b20TemperatureSensorDevice([&record]() {
          Ds18b20TemperatureSensorConfigV1 config{};
          (void)decodeDs18b20TemperatureSensorConfig(record.configPayload, config);
          config.enabled = record.enabled ? 1U : 0U;
          return config;
      }()) {}

Ds18b20TemperatureSensorDevice::Ds18b20TemperatureSensorDevice(const Ds18b20TemperatureSensorConfigV1& config)
    : DeviceRuntimeBase((PState)&Ds18b20TemperatureSensorDevice::Idle), config_(config) {}

const Ds18b20TemperatureSensorConfigV1& Ds18b20TemperatureSensorDevice::config() const {
    return config_;
}

const TemperatureReading& Ds18b20TemperatureSensorDevice::reading() const {
    return reading_;
}

const char* Ds18b20TemperatureSensorDevice::outputStatus() const {
    return outputStatus_;
}

uint8_t Ds18b20TemperatureSensorDevice::consecutiveErrors() const {
    return consecutiveErrors_;
}

uint32_t Ds18b20TemperatureSensorDevice::lastParentGeneration() const {
    return lastParentGeneration_;
}

DeviceTypeDescriptor Ds18b20TemperatureSensorDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kDs18b20TemperatureSensorTypeId;
    descriptor.name = "Ds18b20TemperatureSensorDevice";
    descriptor.currentConfigVersion = kDs18b20TemperatureSensorConfigVersion;
    descriptor.canHaveChildren = false;
    descriptor.maxChildren = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.compatibleParentTypes = {OneWireBusDevice::descriptor().typeId};
    descriptor.createRuntime = &Ds18b20TemperatureSensorDevice::createRuntime;
    descriptor.validateConfig = &Ds18b20TemperatureSensorDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Ds18b20TemperatureSensorDevice::createRuntime(const DeviceRecord& record) {
    return std::unique_ptr<IDeviceRuntime>(new Ds18b20TemperatureSensorDevice(record));
}

DeviceValidationResult Ds18b20TemperatureSensorDevice::validateConfig(const DeviceRecord& record) {
    if (!record.hasParent || record.parentDeviceId == 0U) {
        return {DeviceError::InvalidRelationship, "ds18b20 requires a onewire parent"};
    }
    if (record.configPayload.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "ds18b20 config exceeds supported size"};
    }
    Ds18b20TemperatureSensorConfigV1 config{};
    if (!decodeDs18b20TemperatureSensorConfig(record.configPayload, config)) {
        return {DeviceError::InvalidConfig, "ds18b20 config is invalid"};
    }
    return validateDs18b20TemperatureSensorConfig(config);
}

OneWireBusDevice* Ds18b20TemperatureSensorDevice::parentBus() const {
    if (parentRuntime() == nullptr) {
        return nullptr;
    }
    return static_cast<OneWireBusDevice*>(parentRuntime());
}

bool Ds18b20TemperatureSensorDevice::parentBusReady() const {
    const OneWireBusDevice* parent = parentBus();
    return parent != nullptr && parent->status() == DeviceStatus::Ready;
}

bool Ds18b20TemperatureSensorDevice::parentGenerationChanged() const {
    const OneWireBusDevice* parent = parentBus();
    return parent != nullptr && lastParentGeneration_ != 0U && parent->generation() != lastParentGeneration_;
}

Ds18b20TemperatureSensorDevice::ParentAccessResult
Ds18b20TemperatureSensorDevice::beginParentTransaction(OneWireBusDevice::ChildTransaction& transaction) const {
    OneWireBusDevice* parent = parentBus();
    if (parent == nullptr || parent->status() != DeviceStatus::Ready) {
        return ParentAccessResult::Missing;
    }

    transaction = parent->beginChildTransaction();
    if (!transaction) {
        return ParentAccessResult::Busy;
    }
    return ParentAccessResult::Ready;
}

bool Ds18b20TemperatureSensorDevice::readScratchpad(IOneWireBusDriver& driver, uint8_t (&scratchpad)[kDs18b20ScratchpadSize],
                                                    const char*& error) const {
    if (!driver.reset()) {
        error = kOutputNotFound;
        return false;
    }
    driver.select(config_.address);
    driver.write(kDs18b20CommandReadScratchpad);
    for (size_t index = 0; index < kDs18b20ScratchpadSize; ++index) {
        scratchpad[index] = driver.read();
    }
    if (!ds18b20ScratchpadCrcValid(scratchpad)) {
        error = kOutputCrcError;
        return false;
    }
    return true;
}

bool Ds18b20TemperatureSensorDevice::configureSensor(IOneWireBusDriver& driver, const char*& error) const {
    uint8_t scratchpad[kDs18b20ScratchpadSize]{};
    if (!readScratchpad(driver, scratchpad, error)) {
        return false;
    }

    const uint8_t expectedConfig = ds18b20ResolutionConfigByte(config_.resolution);
    if ((scratchpad[4] & 0x60U) == (expectedConfig & 0x60U)) {
        return true;
    }

    if (!driver.reset()) {
        error = kOutputNotFound;
        return false;
    }
    driver.select(config_.address);
    driver.write(kDs18b20CommandWriteScratchpad);
    driver.write(scratchpad[2]);
    driver.write(scratchpad[3]);
    driver.write(expectedConfig);
    return true;
}

bool Ds18b20TemperatureSensorDevice::requestConversion(IOneWireBusDriver& driver, const char*& error) const {
    if (!driver.reset()) {
        error = kOutputNotFound;
        return false;
    }
    driver.select(config_.address);
    driver.write(kDs18b20CommandConvertT);
    return true;
}

bool Ds18b20TemperatureSensorDevice::readTemperature(IOneWireBusDriver& driver, int32_t& milliCelsius, const char*& error) const {
    uint8_t scratchpad[kDs18b20ScratchpadSize]{};
    if (!readScratchpad(driver, scratchpad, error)) {
        return false;
    }
    if (!ds18b20ParseScratchpadTemperature(scratchpad, milliCelsius)) {
        error = kOutputOutOfRange;
        return false;
    }
    return true;
}

void Ds18b20TemperatureSensorDevice::publishReading(int32_t milliCelsius, uint32_t now) {
    const TemperatureReading previous = reading_;
    reading_.milliCelsius = milliCelsius;
    reading_.measuredAtMs = now;
    reading_.valid = true;
    const bool shouldPublish = config_.reportAlways != 0U ||
                               temperatureReadingChanged(previous, reading_, config_.reportDeltaCentiCelsius) ||
                               std::strcmp(outputStatus_, kOutputOk) != 0;
    outputStatus_ = kOutputOk;
    consecutiveErrors_ = 0;
    if (shouldPublish) {
        markRuntimeStateDirty();
    }
}

void Ds18b20TemperatureSensorDevice::invalidateReading(const char* status) {
    const bool changed = reading_.valid || std::strcmp(outputStatus_, status) != 0;
    reading_.milliCelsius = 0;
    reading_.measuredAtMs = 0;
    reading_.valid = false;
    outputStatus_ = status;
    if (changed) {
        markRuntimeStateDirty();
    }
}

void Ds18b20TemperatureSensorDevice::recordFailure(const char* status, uint32_t now) {
    invalidateReading(status);
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + kRetryBackoffMs;
}

void Ds18b20TemperatureSensorDevice::deferRetry(uint32_t now) {
    retryDeadline_ = now + 100U;
}

TemperatureUnit Ds18b20TemperatureSensorDevice::outputUnit() const {
    TemperatureUnit unit{TemperatureUnit::Celsius};
    (void)temperatureUnitFromByte(config_.outputUnit, unit);
    return unit;
}

SM_STATE(Ds18b20TemperatureSensorDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        invalidateReading(kOutputNotReady);
        SM_GOTO(Starting);
    }
}

SM_STATE(Ds18b20TemperatureSensorDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }

    clearStartRequested();
    invalidateReading(kOutputNotReady);
    SM_GOTO(PowerUpDelay);
}

SM_STATE(Ds18b20TemperatureSensorDevice::PowerUpDelay) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (!elapsed(uptime(), kPowerUpDelayMs)) {
        return;
    }
    SM_GOTO(ConfigureSensor);
}

SM_STATE(Ds18b20TemperatureSensorDevice::ConfigureSensor) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }

    OneWireBusDevice::ChildTransaction transaction;
    const ParentAccessResult parentAccess = beginParentTransaction(transaction);
    if (parentAccess == ParentAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (parentAccess == ParentAccessResult::Busy || transaction.driver() == nullptr) {
        deferRetry(uptime());
        SM_GOTO(RetryBackoff);
    }

    const char* error = kOutputNotReady;
    if (!configureSensor(*transaction.driver(), error)) {
        recordFailure(error, uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }
    lastParentGeneration_ = transaction.generation();
    consecutiveErrors_ = 0;
    SM_GOTO(RequestConversion);
}

SM_STATE(Ds18b20TemperatureSensorDevice::RequestConversion) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (parentGenerationChanged() || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }

    OneWireBusDevice::ChildTransaction transaction;
    const ParentAccessResult parentAccess = beginParentTransaction(transaction);
    if (parentAccess == ParentAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (parentAccess == ParentAccessResult::Busy || transaction.driver() == nullptr) {
        deferRetry(uptime());
        SM_GOTO(RetryBackoff);
    }

    const char* error = kOutputNotReady;
    if (!requestConversion(*transaction.driver(), error)) {
        recordFailure(error, uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    conversionDeadline_ = uptime() + ds18b20ConversionTimeMs(config_.resolution);
    SM_GOTO(WaitConversion);
}

SM_STATE(Ds18b20TemperatureSensorDevice::WaitConversion) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (parentGenerationChanged() || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), conversionDeadline_)) {
        return;
    }
    SM_GOTO(ReadScratchpad);
}

SM_STATE(Ds18b20TemperatureSensorDevice::ReadScratchpad) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (parentGenerationChanged() || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }

    OneWireBusDevice::ChildTransaction transaction;
    const ParentAccessResult parentAccess = beginParentTransaction(transaction);
    if (parentAccess == ParentAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (parentAccess == ParentAccessResult::Busy || transaction.driver() == nullptr) {
        deferRetry(uptime());
        SM_GOTO(RetryBackoff);
    }

    int32_t milliCelsius = 0;
    const char* error = kOutputNotReady;
    if (!readTemperature(*transaction.driver(), milliCelsius, error)) {
        recordFailure(error, uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    publishReading(milliCelsius, uptime());
    nextPollAt_ = uptime() + config_.pollMs;
    SM_GOTO(Ready);
}

SM_STATE(Ds18b20TemperatureSensorDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (parentGenerationChanged() || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), nextPollAt_)) {
        return;
    }
    SM_GOTO(RequestConversion);
}

SM_STATE(Ds18b20TemperatureSensorDevice::RetryBackoff) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), retryDeadline_)) {
        return;
    }
    if (std::strcmp(outputStatus_, kOutputParentBusy) == 0) {
        outputStatus_ = kOutputNotReady;
    }
    SM_GOTO(Starting);
}

SM_STATE(Ds18b20TemperatureSensorDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    invalidateReading(kOutputParentUnavailable);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (parentBusReady() && (reconfigureRequested_ || startRequested_ || lastParentGeneration_ == 0U)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Ds18b20TemperatureSensorDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    lastParentGeneration_ = 0;
    consecutiveErrors_ = 0;
    invalidateReading(kOutputNotReady);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    SM_GOTO(Starting);
}

SM_STATE(Ds18b20TemperatureSensorDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    invalidateReading(kOutputDisabled);
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_ && config_.enabled != 0U) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Ds18b20TemperatureSensorDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        invalidateReading(kOutputNotReady);
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        invalidateReading(kOutputDisabled);
        SM_GOTO(Disabled);
    }
    if (!parentBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputParentUnavailable);
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

SM_STATE(Ds18b20TemperatureSensorDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    invalidateReading(kOutputNotReady);
    setDeleted();
}

} // namespace ewfm
