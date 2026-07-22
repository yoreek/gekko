#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"

#include "debug/Debug.h"
#include "devices/sensors/ds18b20/Ds18b20OneWireProtocol.h"

#include <cmath>
#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Ds18b20TemperatureSensorDevice

namespace {
constexpr uint32_t kPowerUpDelayMs = 20;
constexpr uint32_t kRetryBackoffMs = 1000;
constexpr uint32_t kFaultRetryBackoffMs = 30000;
constexpr uint8_t kFaultErrorThreshold = 3;

const char* kOutputNotReady = "not_ready";
const char* kOutputDependencyUnavailable = "dependency_unavailable";
const char* kOutputDependencyBusy = "dependency_busy";
const char* kOutputNotFound = "not_found";
const char* kOutputCrcError = "crc_error";
const char* kOutputOutOfRange = "out_of_range";
const char* kOutputDisabled = "disabled";
const char* kOutputDuplicateAddress = "duplicate_address";
} // namespace

Ds18b20TemperatureSensorDevice::Ds18b20TemperatureSensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Ds18b20TemperatureSensorDevice([&configBlob]() {
          Ds18b20TemperatureSensorConfigV2 config{};
          (void)decodeDs18b20TemperatureSensorConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Ds18b20TemperatureSensorDevice::Ds18b20TemperatureSensorDevice(const Ds18b20TemperatureSensorConfigV2& config)
    : BusDependentDeviceBase((PState)&Ds18b20TemperatureSensorDevice::Idle), config_(config) {
    filter_.configure(config_.filter);
}

const Ds18b20TemperatureSensorConfigV2& Ds18b20TemperatureSensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Ds18b20TemperatureSensorDevice::baseConfig() const {
    return config_;
}

const TemperatureReading& Ds18b20TemperatureSensorDevice::reading() const {
    return publisher_.reading();
}

const char* Ds18b20TemperatureSensorDevice::outputStatus() const {
    return publisher_.status();
}

uint8_t Ds18b20TemperatureSensorDevice::consecutiveErrors() const {
    return consecutiveErrors_;
}

uint32_t Ds18b20TemperatureSensorDevice::lastDependencyGeneration() const {
    return lastDependencyGeneration_;
}

const ITemperatureReadingRuntime* Ds18b20TemperatureSensorDevice::temperatureReadingRuntime() const {
    return this;
}

bool Ds18b20TemperatureSensorDevice::latestTemperatureReading(TemperatureReading& reading) const {
    reading = publisher_.reading();
    return true;
}

const char* Ds18b20TemperatureSensorDevice::latestTemperatureStatus() const {
    return publisher_.status();
}

void Ds18b20TemperatureSensorDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool Ds18b20TemperatureSensorDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ds18b20TemperatureSensorConfigSize(config_);
    return encodeFixedConfigBlob(Ds18b20TemperatureSensorConfigV2::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Ds18b20TemperatureSensorDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Ds18b20TemperatureSensorConfigV2 config{};
    if (!decodeDs18b20TemperatureSensorConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool addressChanged = std::memcmp(config.address.bytes, config_.address.bytes, sizeof(config.address.bytes)) != 0;
    const bool resolutionChanged = config.resolution != config_.resolution;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = addressChanged || resolutionChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Ds18b20TemperatureSensorDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    Ds18b20TemperatureSensorConfigV2 config{};
    if (!decodeDs18b20TemperatureSensorConfig(configBlob.data(), configBlob.size(), config)) {
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
DeviceTypeDescriptor Ds18b20TemperatureSensorDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kDs18b20TemperatureSensorTypeId;
    descriptor.name = "Ds18b20TemperatureSensorDevice";
    descriptor.currentConfigVersion = kDs18b20TemperatureSensorConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::OneWireBus, true}};
    descriptor.providedRoles = ProvidedRoles::of({ITemperatureReadingRuntime::kProvidedRole});
    descriptor.createRuntime = &Ds18b20TemperatureSensorDevice::createRuntime;
    descriptor.validateConfig = &Ds18b20TemperatureSensorDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Ds18b20TemperatureSensorDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                              const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Ds18b20TemperatureSensorDevice(record, configBlob));
}

DeviceValidationResult Ds18b20TemperatureSensorDevice::validateConfig(const DeviceRegistryEntry& record,
                                                                      const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::OneWireBus) == 0U) {
        return {DeviceError::InvalidRelationship, "ds18b20 requires a onewire dependency"};
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "ds18b20 config exceeds supported size"};
    }
    Ds18b20TemperatureSensorConfigV2 config{};
    if (!decodeDs18b20TemperatureSensorConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "ds18b20 config is invalid"};
    }
    return config.validate();
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

bool Ds18b20TemperatureSensorDevice::oneWireRomAddress(OneWireRomAddress& address) const {
    address = config_.address;
    return true;
}

bool Ds18b20TemperatureSensorDevice::hasDuplicateOneWireAddress() const {
    const OneWireBusDevice* dependency = dependencyBus();
    return dependency != nullptr && dependency->hasDuplicateDependentRomAddress(config_.address, this);
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
    const int32_t filtered = static_cast<int32_t>(std::lround(filter_.apply(static_cast<float>(milliCelsius))));
    publisher_.configure(config_.reportAlways != 0U, config_.reportDeltaCentiCelsius);
    consecutiveErrors_ = 0;
    if (publisher_.publish(filtered, now)) {
        markRuntimeStateDirty();
    }
}

void Ds18b20TemperatureSensorDevice::invalidateReading(const char* status) {
    if (publisher_.invalidate(status)) {
        markRuntimeStateDirty();
    }
}

void Ds18b20TemperatureSensorDevice::recordFailure(const char* status, uint32_t now) {
    invalidateReading(status);
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = consecutiveErrors_ >= kFaultErrorThreshold ? now + kFaultRetryBackoffMs : now + kRetryBackoffMs;
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
    }

    clearStartRequested();
    filter_.reset();
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
    }

    OneWireBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
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
    lastDependencyGeneration_ = transaction.generation();
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
    }

    OneWireBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
    }

    OneWireBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), retryDeadline_)) {
        return;
    }
    if (std::strcmp(publisher_.status(), kOutputDependencyBusy) == 0) {
        publisher_.setStatusQuietly(kOutputNotReady);
    }
    SM_GOTO(Starting);
}

SM_STATE(Ds18b20TemperatureSensorDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    invalidateReading(kOutputDependencyUnavailable);
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
    if (dependencyBusReady() && (reconfigureRequested_ || startRequested_ || lastDependencyGeneration_ == 0U)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Ds18b20TemperatureSensorDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    lastDependencyGeneration_ = 0;
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
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
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        invalidateReading(kOutputDependencyUnavailable);
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        clearFaultRequested();
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (hasDuplicateOneWireAddress()) {
        status_ = DeviceStatus::Faulted;
        invalidateReading(kOutputDuplicateAddress);
        requestFault();
        SM_GOTO(Faulted);
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
