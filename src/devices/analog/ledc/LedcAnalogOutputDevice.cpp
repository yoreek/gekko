#include "devices/analog/ledc/LedcAnalogOutputDevice.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#endif

#include "devices/core/ConfigCodec.h"

namespace ewfm {

LedcAnalogOutputDevice::LedcAnalogOutputDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : LedcAnalogOutputDevice([&configBlob]() {
          LedcAnalogOutputDeviceConfigV1 config{};
          (void)decodeLedcAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

LedcAnalogOutputDevice::LedcAnalogOutputDevice(const LedcAnalogOutputDeviceConfigV1& config)
    : AnalogOutputDeviceBase(config), config_(config) {}

const LedcAnalogOutputDeviceConfigV1& LedcAnalogOutputDevice::config() const {
    return config_;
}

bool LedcAnalogOutputDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ledcAnalogOutputDeviceConfigSize(config_);
    return encodeFixedConfigBlob(LedcAnalogOutputDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan LedcAnalogOutputDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    LedcAnalogOutputDeviceConfigV1 next{};
    if (!decodeLedcAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = next.pin != config_.pin || next.ledcChannel != config_.ledcChannel || next.frequencyHz != config_.frequencyHz ||
                        next.dutyBits != config_.dutyBits || next.inverted != config_.inverted;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool LedcAnalogOutputDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    LedcAnalogOutputDeviceConfigV1 next{};
    if (!decodeLedcAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return false;
    }
    config_ = next;
    return true;
}

DeviceTypeDescriptor LedcAnalogOutputDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kLedcAnalogOutputDeviceTypeId;
    descriptor.name = "LedcAnalogOutputDevice";
    descriptor.currentConfigVersion = kLedcAnalogOutputDeviceConfigVersion;
    descriptor.maxDependents = 8;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.ticks100ms = false;
    descriptor.ticks1s = false;
    descriptor.providedRoles = ProvidedRoles::of({IAnalogOutputRuntime::kProvidedRole});
    descriptor.createRuntime = &LedcAnalogOutputDevice::createRuntime;
    descriptor.validateConfig = &LedcAnalogOutputDevice::validateConfig;
    return descriptor;
}

void LedcAnalogOutputDevice::claimGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.pin, deviceId());
}

void LedcAnalogOutputDevice::releaseGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.pin, 0);
}

std::unique_ptr<IDeviceRuntime> LedcAnalogOutputDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                      const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new LedcAnalogOutputDevice(record, configBlob));
}

DeviceValidationResult LedcAnalogOutputDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    return validateOutputConfig<LedcAnalogOutputDeviceConfigV1>(configBlob, decodeLedcAnalogOutputDeviceConfig);
}

DeviceValidationResult LedcAnalogOutputDevice::configureHardware(uint32_t now) {
    (void)now;
    const DeviceValidationResult validation = config_.validate();
    if (!validation.ok()) {
        return validation;
    }
#if defined(ARDUINO) && !defined(UNIT_TEST)
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    (void)ledcAttachChannel(config_.pin, config_.frequencyHz, config_.dutyBits, config_.ledcChannel);
#else
    (void)ledcSetup(config_.ledcChannel, config_.frequencyHz, config_.dutyBits);
    (void)ledcAttachPin(config_.pin, config_.ledcChannel);
#endif
#endif
    hardwareReady_ = true;
    return {};
}

DeviceValidationResult LedcAnalogOutputDevice::applyHardwareOutput(const uint16_t state, uint32_t now) {
    (void)now;
    if (!hardwareReady_) {
        return {DeviceError::StorageError, "ledc analog output hardware is not ready"};
    }
    const uint32_t duty = outputStateToDuty(state);
#if defined(ARDUINO) && !defined(UNIT_TEST)
#if ESP_ARDUINO_VERSION_MAJOR >= 3
    (void)ledcWriteChannel(config_.ledcChannel, duty);
#else
    (void)ledcWrite(config_.ledcChannel, duty);
#endif
#endif
    (void)duty;
    return {};
}

void LedcAnalogOutputDevice::releaseHardware(uint32_t now) {
    (void)now;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (hardwareReady_) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
        (void)ledcWriteChannel(config_.ledcChannel, 0U);
        (void)ledcDetach(config_.pin);
#else
        (void)ledcWrite(config_.ledcChannel, 0U);
        (void)ledcDetachPin(config_.pin);
#endif
    }
    pinMode(config_.pin, INPUT);
#endif
    hardwareReady_ = false;
}

uint32_t LedcAnalogOutputDevice::dutyMax() const {
    return config_.dutyBits >= 31U ? 0xFFFFFFFFUL : ((1UL << config_.dutyBits) - 1UL);
}

uint32_t LedcAnalogOutputDevice::outputStateToDuty(const uint16_t state) const {
    const uint32_t maximum = dutyMax();
    return static_cast<uint32_t>((static_cast<uint64_t>(state) * maximum) / kAnalogOutputLevelMax);
}

} // namespace ewfm
