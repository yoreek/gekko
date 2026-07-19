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
constexpr uint8_t kMaxEsp32OutputPin = 33;
constexpr uint8_t kFlashPinStart = 6;
constexpr uint8_t kFlashPinEnd = 11;

} // namespace

static_assert(std::is_trivially_copyable<SwitchDeviceConfigV1>::value, "SwitchDeviceConfigV1 must be POD");
static_assert(std::is_trivially_copyable<GpioSwitchDeviceConfigV1>::value, "GpioSwitchDeviceConfigV1 must be POD");
static_assert(std::is_trivially_copyable<GpioSwitchDevicePersistedConfigV1>::value, "GpioSwitchDevicePersistedConfigV1 must be POD");
static_assert(std::is_trivially_copyable<GpioSwitchDeviceConfigV2>::value, "GpioSwitchDeviceConfigV2 must be POD");
static_assert(std::is_trivially_copyable<GpioSwitchDeviceConfigV3>::value, "GpioSwitchDeviceConfigV3 must be POD");
static_assert(sizeof(SwitchDeviceConfigV1) == 38, "SwitchDeviceConfigV1 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV1) == 1, "GpioSwitchDeviceConfigV1 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV2) == 39, "GpioSwitchDeviceConfigV2 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV3) == 39, "GpioSwitchDeviceConfigV3 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV3::kMagic) - 1U + sizeof(GpioSwitchDeviceConfigV3) <= kMaxDeviceConfigBytes,
              "GpioSwitchDeviceConfigV3 exceeds device config bound");

bool decodeGpioSwitchDeviceConfig(const uint8_t* blob, size_t size, GpioSwitchDeviceConfigV3& config) {
    if (decodeValidatedFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, blob, size, config)) {
        return true;
    }

    GpioSwitchDeviceConfigV2 legacyV2{};
    if (decodeFixedConfigBlob(GpioSwitchDeviceConfigV2::kMagic, blob, size, legacyV2)) {
        if (!legacyV2.SwitchDeviceConfigV1::validate().ok() || !gpioSwitchPinIsValid(legacyV2.gpioPin)) {
            return false;
        }
        config.migrateFrom(legacyV2);
        return config.validate().ok();
    }

    GpioSwitchDevicePersistedConfigV1 legacy{};
    size_t pos = 0;
    if (!readFixedConfigSegment(SwitchDeviceConfigV1::kMagic, blob, size, pos, legacy.switchConfig) ||
        !readFixedConfigSegment(GpioSwitchDeviceConfigV1::kMagic, blob, size, pos, legacy.gpioConfig) || pos != size) {
        return false;
    }
    if (!legacy.switchConfig.validate().ok() || !gpioSwitchPinIsValid(legacy.gpioConfig.gpioPin)) {
        return false;
    }
    legacyV2 = {};
    static_cast<SwitchDeviceConfigV1&>(legacyV2) = legacy.switchConfig;
    legacyV2.gpioPin = legacy.gpioConfig.gpioPin;
    config.migrateFrom(legacyV2);
    return config.validate().ok();
}

bool GpioSwitchDeviceConfigV3::parseJson(const JsonObjectConst& input, const char*& error) {
    if (!SwitchDeviceConfigV2::parseJson(input, error)) {
        return false;
    }

    gpioPin = static_cast<uint8_t>(input["gpioPin"] | static_cast<int>(gpioPin));
    if (!gpioSwitchPinIsValid(gpioPin)) {
        error = "gpio switch pin is invalid";
        return false;
    }
    return true;
}

DeviceValidationResult GpioSwitchDeviceConfigV3::validate() const {
    const DeviceValidationResult switchResult = SwitchDeviceConfigV2::validate();
    if (!switchResult.ok()) {
        return switchResult;
    }
    return gpioSwitchPinIsValid(gpioPin) ? DeviceValidationResult{}
                                         : DeviceValidationResult{DeviceError::InvalidConfig, "gpio switch pin is invalid"};
}

void GpioSwitchDeviceConfigV3::writeJson(JsonObject output) const {
    SwitchDeviceConfigV2::writeJson(output);
    output["gpioPin"] = gpioPin;
}

void GpioSwitchDeviceConfigV3::migrateFrom(const GpioSwitchDeviceConfigV2& legacy) {
    SwitchDeviceConfigV2::migrateFrom(legacy);
    gpioPin = legacy.gpioPin;
}

bool gpioSwitchPinIsValid(uint8_t pin) {
    if (pin > kMaxEsp32OutputPin) {
        return false;
    }
    return pin < kFlashPinStart || pin > kFlashPinEnd;
}

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
