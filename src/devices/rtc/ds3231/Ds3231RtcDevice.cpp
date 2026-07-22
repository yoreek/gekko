#include "devices/rtc/ds3231/Ds3231RtcDevice.h"

#include "debug/Debug.h"
#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "time/DateTime.h"

#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS Ds3231RtcDevice

namespace {
constexpr uint8_t kRegTime = 0x00;
constexpr uint8_t kRegStatus = 0x0F;
constexpr uint8_t kOscillatorStopFlagBit = 0x80;

constexpr uint32_t kPollMs = 1000;
constexpr uint32_t kBusyRetryMs = 100;
constexpr uint32_t kRetryBackoffMs = 1000;
constexpr uint32_t kFaultRetryBackoffMs = 30000;
constexpr uint8_t kFaultErrorThreshold = 3;

uint8_t bcd2bin(uint8_t val) {
    return static_cast<uint8_t>(val - 6 * (val >> 4));
}

uint8_t bin2bcd(uint8_t val) {
    return static_cast<uint8_t>(val + 6 * (val / 10));
}
} // namespace

Ds3231RtcDevice::Ds3231RtcDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Ds3231RtcDevice([&configBlob]() {
          Ds3231RtcDeviceConfigV2 config{};
          (void)decodeDs3231RtcDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Ds3231RtcDevice::Ds3231RtcDevice(const Ds3231RtcDeviceConfigV2& config)
    : Ds3231RtcDeviceBase((PState)&Ds3231RtcDevice::Idle), config_(config) {}

const Ds3231RtcDeviceConfigV2& Ds3231RtcDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& Ds3231RtcDevice::baseConfig() const {
    return config_;
}

const IRealTimeClockRuntime* Ds3231RtcDevice::realTimeClockRuntime() const {
    return this;
}

bool Ds3231RtcDevice::latestTimeReading(uint32_t& outUtcEpoch, bool& outLostPower) const {
    if (!hasReading_) {
        return false;
    }
    outUtcEpoch = lastEpoch_;
    outLostPower = lastLostPower_;
    return true;
}

bool Ds3231RtcDevice::writeTime(uint32_t utcEpoch) {
    I2cBusDevice::DependencyTransaction transaction;
    if (beginDependencyTransaction(transaction) != DependencyAccessResult::Ready || transaction.driver() == nullptr) {
        return false;
    }
    return writeTimeRegisters(*transaction.driver(), utcEpoch);
}

bool Ds3231RtcDevice::useForSystemTimeSync() const {
    return config_.useForSystemTimeSync != 0U;
}

bool Ds3231RtcDevice::lastReadOk() const {
    return lastReadOk_;
}

void Ds3231RtcDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool Ds3231RtcDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ds3231RtcDeviceConfigSize(config_);
    return encodeFixedConfigBlob(Ds3231RtcDeviceConfigV2::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Ds3231RtcDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Ds3231RtcDeviceConfigV2 config{};
    if (!decodeDs3231RtcDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    const bool addressChanged = config.i2cAddress != config_.i2cAddress;
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = addressChanged;
    plan.resetStateMachine = addressChanged;
    return plan;
}

bool Ds3231RtcDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Ds3231RtcDeviceConfigV2 config{};
    if (!decodeDs3231RtcDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}
DeviceTypeDescriptor Ds3231RtcDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kDs3231RtcTypeId;
    descriptor.name = "Ds3231RtcDevice";
    descriptor.currentConfigVersion = kDs3231RtcConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::I2CBus, true}};
    descriptor.providedRoles = ProvidedRoles::of({IRealTimeClockRuntime::kProvidedRole});
    descriptor.createRuntime = &Ds3231RtcDevice::createRuntime;
    descriptor.validateConfig = &Ds3231RtcDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Ds3231RtcDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Ds3231RtcDevice(record, configBlob));
}

DeviceValidationResult Ds3231RtcDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return validateI2cConfig<Ds3231RtcDeviceConfigV2>(record, configBlob, decodeDs3231RtcDeviceConfig);
}

bool Ds3231RtcDevice::readTimeRegisters(II2cBusDriver& driver, uint32_t& outEpoch, bool& outLostPower) const {
    driver.beginTransmission(config_.i2cAddress);
    driver.write(kRegTime);
    if (driver.endTransmission(false) != 0U) {
        return false;
    }
    uint8_t buffer[7]{};
    if (driver.requestFrom(config_.i2cAddress, sizeof(buffer), true) != sizeof(buffer)) {
        return false;
    }
    for (uint8_t& value : buffer) {
        value = static_cast<uint8_t>(driver.read());
    }

    const uint8_t second = bcd2bin(buffer[0] & 0x7FU);
    const uint8_t minute = bcd2bin(buffer[1]);
    const uint8_t hour = bcd2bin(buffer[2]);
    const uint8_t day = bcd2bin(buffer[4]);
    const uint8_t month = bcd2bin(buffer[5] & 0x7FU);
    const uint16_t year = static_cast<uint16_t>(bcd2bin(buffer[6]) + 2000U);

    const DateTime dt(year, month, day, hour, minute, second);
    outEpoch = dt.utcUnixtime();

    driver.beginTransmission(config_.i2cAddress);
    driver.write(kRegStatus);
    if (driver.endTransmission(false) != 0U) {
        return false;
    }
    if (driver.requestFrom(config_.i2cAddress, 1, true) != 1U) {
        return false;
    }
    const uint8_t statusReg = static_cast<uint8_t>(driver.read());
    outLostPower = (statusReg & kOscillatorStopFlagBit) != 0U;
    return true;
}

bool Ds3231RtcDevice::writeTimeRegisters(II2cBusDriver& driver, uint32_t utcEpoch) const {
    const DateTime dt(utcEpoch);
    uint8_t buffer[8] = {
        kRegTime,
        bin2bcd(dt.second()),
        bin2bcd(dt.minute()),
        bin2bcd(dt.hour()),
        bin2bcd(dt.weekday()),
        bin2bcd(dt.day()),
        bin2bcd(dt.month()),
        bin2bcd(static_cast<uint8_t>(dt.year() - 2000U)),
    };
    driver.beginTransmission(config_.i2cAddress);
    driver.write(buffer, sizeof(buffer));
    if (driver.endTransmission(true) != 0U) {
        return false;
    }

    driver.beginTransmission(config_.i2cAddress);
    driver.write(kRegStatus);
    if (driver.endTransmission(false) != 0U) {
        return false;
    }
    if (driver.requestFrom(config_.i2cAddress, 1, true) != 1U) {
        return false;
    }
    const uint8_t statusReg = static_cast<uint8_t>(driver.read());
    driver.beginTransmission(config_.i2cAddress);
    driver.write(kRegStatus);
    driver.write(static_cast<uint8_t>(statusReg & static_cast<uint8_t>(~kOscillatorStopFlagBit)));
    return driver.endTransmission(true) == 0U;
}

void Ds3231RtcDevice::recordSuccess(uint32_t epoch, bool lostPower) {
    const bool changed = !hasReading_ || !lastReadOk_ || lastEpoch_ != epoch || lastLostPower_ != lostPower;
    lastEpoch_ = epoch;
    lastLostPower_ = lostPower;
    hasReading_ = true;
    lastReadOk_ = true;
    consecutiveErrors_ = 0;
    if (changed) {
        markRuntimeStateDirty();
    }
}

void Ds3231RtcDevice::recordFailure(uint32_t now) {
    if (lastReadOk_) {
        lastReadOk_ = false;
        markRuntimeStateDirty();
    }
    if (consecutiveErrors_ < 255U) {
        ++consecutiveErrors_;
    }
    retryDeadline_ = now + (consecutiveErrors_ >= kFaultErrorThreshold ? kFaultRetryBackoffMs : kRetryBackoffMs);
}

SM_STATE(Ds3231RtcDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(Ds3231RtcDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }

    I2cBusDevice::DependencyTransaction transaction;
    const DependencyAccessResult dependencyAccess = beginDependencyTransaction(transaction);
    if (dependencyAccess == DependencyAccessResult::Missing) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    if (!probeI2cPresence(*transaction.driver())) {
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    lastDependencyGeneration_ = transaction.generation();
    clearStartRequested();
    SM_GOTO(Reading);
}

SM_STATE(Ds3231RtcDevice::Reading) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
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
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyAccess == DependencyAccessResult::Busy || transaction.driver() == nullptr) {
        retryDeadline_ = uptime() + kBusyRetryMs;
        SM_GOTO(RetryBackoff);
    }

    uint32_t epoch = 0;
    bool lostPower = false;
    if (!readTimeRegisters(*transaction.driver(), epoch, lostPower)) {
        recordFailure(uptime());
        if (consecutiveErrors_ >= kFaultErrorThreshold) {
            SM_GOTO(Faulted);
        }
        SM_GOTO(RetryBackoff);
    }

    recordSuccess(epoch, lostPower);
    nextPollAt_ = uptime() + kPollMs;
    SM_GOTO(Ready);
}

SM_STATE(Ds3231RtcDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (dependencyGenerationChanged(lastDependencyGeneration_) || reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (!EWFM_SM_TIME_REACHED(uptime(), nextPollAt_)) {
        return;
    }
    SM_GOTO(Reading);
}

SM_STATE(Ds3231RtcDevice::RetryBackoff) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
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

SM_STATE(Ds3231RtcDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBusReady() && (reconfigureRequested_ || startRequested_ || lastDependencyGeneration_ == 0U)) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(Ds3231RtcDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    clearStartRequested();
    lastDependencyGeneration_ = 0;
    consecutiveErrors_ = 0;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    SM_GOTO(Starting);
}

SM_STATE(Ds3231RtcDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
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

SM_STATE(Ds3231RtcDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!dependencyBusReady()) {
        status_ = DeviceStatus::DependencyBlocked;
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

SM_STATE(Ds3231RtcDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
