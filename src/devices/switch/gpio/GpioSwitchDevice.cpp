#include "devices/switch/gpio/GpioSwitchDevice.h"

#include "devices/core/ConfigCodec.h"
#include "devices/switch/gpio/ArduinoGpioOutputDriver.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS GpioSwitchDevice

namespace {
constexpr DeviceTypeId kGpioSwitchDeviceTypeId = 2;
constexpr uint32_t kGpioSwitchDeviceConfigVersion = 3;
} // namespace

GpioSwitchDevice::GpioSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : GpioSwitchDevice(
          [&configBlob]() {
              GpioSwitchDeviceConfigV3 config{};
              (void)decodeGpioSwitchDeviceConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          defaultArduinoGpioOutputDriver()) {
    bindDeviceIdentity(record, configBlob);
}

GpioSwitchDevice::GpioSwitchDevice(const GpioSwitchDeviceConfigV3& config, IGpioOutputDriver& driver)
    : SwitchDeviceBase(config), config_(config), driver_(driver) {}

uint8_t GpioSwitchDevice::gpioPin() const {
    return config_.gpioPin;
}

const GpioSwitchDeviceConfigV3& GpioSwitchDevice::config() const {
    return config_;
}

bool GpioSwitchDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = gpioSwitchDeviceConfigSize(config_);
    return encodeFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan GpioSwitchDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    GpioSwitchDeviceConfigV3 config{};
    if (!decodeGpioSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool invertedChanged = config.inverted != config_.inverted;
    const bool gpioPinChanged = config.gpioPin != config_.gpioPin;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = gpioPinChanged || invertedChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool GpioSwitchDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    GpioSwitchDeviceConfigV3 config{};
    if (!decodeGpioSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

SwitchDeviceConfigV2& GpioSwitchDevice::mutableConfig() {
    return config_;
}
DeviceTypeDescriptor GpioSwitchDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kGpioSwitchDeviceTypeId;
    descriptor.name = "GpioSwitchDevice";
    descriptor.currentConfigVersion = kGpioSwitchDeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.providedRoles = ProvidedRoles::of({ISwitchOutputRuntime::kProvidedRole, IStatusRuntime::kProvidedRole});
    descriptor.createRuntime = &GpioSwitchDevice::createRuntime;
    descriptor.validateConfig = &GpioSwitchDevice::validateConfig;
    return descriptor;
}

void GpioSwitchDevice::claimGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.gpioPin, deviceId());
}

void GpioSwitchDevice::releaseGpioPins(DeviceId* pins) const {
    setGpioPinOwner(pins, config_.gpioPin, 0);
}

std::unique_ptr<IDeviceRuntime> GpioSwitchDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new GpioSwitchDevice(record, configBlob));
}

DeviceValidationResult GpioSwitchDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    return validateOutputConfig<GpioSwitchDeviceConfigV3>(configBlob, decodeGpioSwitchDeviceConfig);
}

DeviceValidationResult GpioSwitchDevice::configureHardware(uint32_t now) {
    (void)now;
    if (!gpioSwitchPinIsValid(config_.gpioPin)) {
        return {DeviceError::InvalidConfig, "gpio switch pin is invalid"};
    }

    const bool physicalLevel = config_.startupState ? !config_.inverted : config_.inverted;
    return driver_.configureOutput(config_.gpioPin, physicalLevel)
               ? DeviceValidationResult{}
               : DeviceValidationResult{DeviceError::StorageError, "gpio configure failed"};
}

DeviceValidationResult GpioSwitchDevice::applyHardwareOutput(const bool state, uint32_t now) {
    (void)now;
    return driver_.write(config_.gpioPin, state) ? DeviceValidationResult{}
                                                 : DeviceValidationResult{DeviceError::StorageError, "gpio write failed"};
}

void GpioSwitchDevice::releaseHardware(uint32_t now) {
    (void)now;
    driver_.release(config_.gpioPin);
}

} // namespace ewfm
