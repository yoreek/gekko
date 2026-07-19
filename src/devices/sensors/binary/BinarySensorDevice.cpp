#include "devices/sensors/binary/BinarySensorDevice.h"

#include "devices/sensors/binary/ArduinoGpioInputDriver.h"

#include <cstring>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS BinarySensorDevice

BinarySensorDevice::BinarySensorDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : BinarySensorDevice(
          [&configBlob]() {
              BinarySensorDeviceConfigV1 config{};
              (void)decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          defaultArduinoGpioInputDriver()) {
    bindDeviceIdentity(record, configBlob);
}

BinarySensorDevice::BinarySensorDevice(const BinarySensorDeviceConfigV1& config, IGpioInputDriver& driver)
    : DeviceRuntimeBase((PState)&BinarySensorDevice::Idle), config_(config), driver_(driver) {}

const BinarySensorDeviceConfigV1& BinarySensorDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& BinarySensorDevice::baseConfig() const {
    return config_;
}

bool BinarySensorDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = binarySensorDeviceConfigSize(config_);
    return encodeFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan BinarySensorDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    BinarySensorDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool pinChanged = config.gpioPin != config_.gpioPin;
    const bool pullModeChanged = config.pullMode != config_.pullMode;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = pinChanged || pullModeChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool BinarySensorDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    BinarySensorDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    const bool invertedChanged = config.inverted != config_.inverted;
    config_ = config;
    if (invertedChanged) {
        markRuntimeStateDirty();
    }
    return true;
}
bool BinarySensorDevice::isActive() const {
    if (!hasStableLevel_) {
        return false;
    }
    return stableLevel_ != (config_.inverted != 0U);
}

bool BinarySensorDevice::hasReading() const {
    return hasStableLevel_;
}

bool BinarySensorDevice::rawLevel() const {
    return rawLevel_;
}

DeviceTypeDescriptor BinarySensorDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kBinarySensorDeviceTypeId;
    descriptor.name = "BinarySensorDevice";
    descriptor.currentConfigVersion = kBinarySensorDeviceConfigVersion;
    descriptor.maxDependents = 8;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.providedRoles = ProvidedRoles::of({IStatusRuntime::kProvidedRole});
    descriptor.createRuntime = &BinarySensorDevice::createRuntime;
    descriptor.validateConfig = &BinarySensorDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> BinarySensorDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new BinarySensorDevice(record, configBlob));
}

DeviceValidationResult BinarySensorDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "binary sensor config exceeds supported size"};
    }
    BinarySensorDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "binary sensor config is invalid"};
    }
    return config.validate();
}

bool BinarySensorDevice::configureHardware() {
    resetSampling();
    hardwareConfigured_ = driver_.configureInput(config_.gpioPin, static_cast<GpioInputPullMode>(config_.pullMode));
    return hardwareConfigured_;
}

void BinarySensorDevice::releaseHardware() {
    if (hardwareConfigured_) {
        driver_.release(config_.gpioPin);
        hardwareConfigured_ = false;
    }
    resetSampling();
}

void BinarySensorDevice::resetSampling() {
    hasStableLevel_ = false;
    stableLevel_ = false;
    candidateLevel_ = false;
    candidateSinceMs_ = 0U;
    rawLevel_ = false;
}

void BinarySensorDevice::sampleInput(uint32_t now) {
    bool raw = false;
    if (!driver_.read(config_.gpioPin, raw)) {
        return;
    }
    rawLevel_ = raw;

    // First successful read seeds the stable level immediately - a fresh sensor should report a
    // real value right away, the debounce window only guards subsequent transitions.
    if (!hasStableLevel_) {
        hasStableLevel_ = true;
        stableLevel_ = raw;
        candidateLevel_ = raw;
        candidateSinceMs_ = now;
        markRuntimeStateDirty();
        return;
    }

    if (raw != candidateLevel_) {
        candidateLevel_ = raw;
        candidateSinceMs_ = now;
        return;
    }

    if (candidateLevel_ != stableLevel_ && static_cast<uint32_t>(now - candidateSinceMs_) >= config_.debounceMs) {
        stableLevel_ = candidateLevel_;
        markRuntimeStateDirty();
    }
}

SM_STATE(BinarySensorDevice::Idle) {
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

SM_STATE(BinarySensorDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!configureHardware()) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }

    clearStartRequested();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(BinarySensorDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        SM_GOTO(Reconfiguring);
    }
    sampleInput(uptime());
}

SM_STATE(BinarySensorDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        releaseHardware();
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (!configureHardware()) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(BinarySensorDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (!disableRequested_ && config_.enabled != 0U) {
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(BinarySensorDevice::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware();
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(BinarySensorDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
