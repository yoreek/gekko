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
constexpr uint32_t kGpioSwitchDeviceConfigVersion = 1;
constexpr uint8_t kMaxEsp32OutputPin = 33;
constexpr uint8_t kFlashPinStart = 6;
constexpr uint8_t kFlashPinEnd = 11;

bool parseOutputState(const char* value, OutputState& state, const char*& error) {
    if (value == nullptr || std::strcmp(value, "off") == 0) {
        state = OutputState::Off;
        return true;
    }
    if (std::strcmp(value, "on") == 0) {
        state = OutputState::On;
        return true;
    }
    if (std::strcmp(value, "disabled") == 0) {
        state = OutputState::Disabled;
        return true;
    }
    error = "unsupported gpio switch output state";
    return false;
}

} // namespace

static_assert(std::is_trivially_copyable<SwitchDeviceConfigV1>::value, "SwitchDeviceConfigV1 must be POD");
static_assert(std::is_trivially_copyable<GpioSwitchDeviceConfigV1>::value, "GpioSwitchDeviceConfigV1 must be POD");
static_assert(sizeof(SwitchDeviceConfigV1) == 38, "SwitchDeviceConfigV1 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV1) == 1, "GpioSwitchDeviceConfigV1 layout changed");
static_assert(sizeof(SwitchDeviceConfigV1::kMagic) - 1U + sizeof(SwitchDeviceConfigV1) + sizeof(GpioSwitchDeviceConfigV1::kMagic) - 1U +
                      sizeof(GpioSwitchDeviceConfigV1) <=
                  kMaxDeviceConfigBytes,
              "GpioSwitchDeviceConfigV1 exceeds device config bound");

bool encodeGpioSwitchDeviceConfig(const GpioSwitchDevicePersistedConfigV1& config, uint8_t* blob, size_t capacity) {
    size_t pos = 0;
    if (!appendFixedConfigSegment(SwitchDeviceConfigV1::kMagic, config.switchConfig, blob, capacity, pos) ||
        !appendFixedConfigSegment(GpioSwitchDeviceConfigV1::kMagic, config.gpioConfig, blob, capacity, pos)) {
        return false;
    }
    return true;
}

bool decodeGpioSwitchDeviceConfig(const uint8_t* blob, size_t size, GpioSwitchDevicePersistedConfigV1& config) {
    size_t pos = 0;
    if (!readFixedConfigSegment(SwitchDeviceConfigV1::kMagic, blob, size, pos, config.switchConfig) ||
        !readFixedConfigSegment(GpioSwitchDeviceConfigV1::kMagic, blob, size, pos, config.gpioConfig)) {
        return false;
    }
    return validateSwitchDeviceConfig(config.switchConfig).ok() && gpioSwitchPinIsValid(config.gpioConfig.gpioPin);
}

bool parseGpioSwitchDeviceConfigJson(const JsonObjectConst& input, GpioSwitchDevicePersistedConfigV1& config, const char*& error) {
    config.switchConfig.restorePreviousState = (input["restore_previous_state"] | false) ? 1U : 0U;
    config.switchConfig.inverted = (input["inverted"] | false) ? 1U : 0U;

    OutputState startup{};
    OutputState safe{};
    if (!parseOutputState(input["startup_state"] | "off", startup, error) || !parseOutputState(input["safe_state"] | "off", safe, error)) {
        return false;
    }
    config.switchConfig.startupState = static_cast<uint8_t>(startup);
    config.switchConfig.safeState = static_cast<uint8_t>(safe);

    config.gpioConfig.gpioPin = static_cast<uint8_t>(input["gpio_pin"] | static_cast<int>(config.gpioConfig.gpioPin));
    if (!gpioSwitchPinIsValid(config.gpioConfig.gpioPin)) {
        error = "gpio switch pin is invalid";
        return false;
    }
    return true;
}

void writeGpioSwitchDeviceConfigJson(const GpioSwitchDevicePersistedConfigV1& config, JsonObject output) {
    OutputState startup{};
    OutputState safe{};
    (void)outputStateFromByte(config.switchConfig.startupState, startup);
    (void)outputStateFromByte(config.switchConfig.safeState, safe);
    writeDeviceBaseConfigJson(config.switchConfig.base, output);
    output["restore_previous_state"] = config.switchConfig.restorePreviousState != 0U;
    output["startup_state"] = outputStateName(startup);
    output["safe_state"] = outputStateName(safe);
    output["inverted"] = config.switchConfig.inverted != 0U;
    output["gpio_pin"] = config.gpioConfig.gpioPin;
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
              GpioSwitchDevicePersistedConfigV1 config{};
              (void)decodeGpioSwitchDeviceConfig(configBlob.data(), configBlob.size(), config);
              return config;
          }(),
          defaultArduinoGpioOutputDriver()) {
    bindDeviceIdentity(record, configBlob);
}

GpioSwitchDevice::GpioSwitchDevice(const GpioSwitchDevicePersistedConfigV1& config, IGpioOutputDriver& driver)
    : TriStateSwitchDeviceBase(config.switchConfig), config_(config.gpioConfig), driver_(driver) {}

uint8_t GpioSwitchDevice::gpioPin() const {
    return config_.gpioPin;
}

const GpioSwitchDeviceConfigV1& GpioSwitchDevice::gpioConfig() const {
    return config_;
}

bool GpioSwitchDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    GpioSwitchDevicePersistedConfigV1 config{};
    config.switchConfig = switchConfig();
    config.gpioConfig = config_;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = gpioSwitchDeviceConfigSize(config);
    return encodeGpioSwitchDeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

bool GpioSwitchDevice::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    GpioSwitchDevicePersistedConfigV1 config{};
    config.switchConfig = switchConfig();
    config.switchConfig.base = baseConfig;
    config.gpioConfig = config_;
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = gpioSwitchDeviceConfigSize(config);
    return encodeGpioSwitchDeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan GpioSwitchDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    GpioSwitchDevicePersistedConfigV1 config{};
    if (!decodeGpioSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool invertedChanged = config.switchConfig.inverted != switchConfig().inverted;
    const bool gpioPinChanged = config.gpioConfig.gpioPin != config_.gpioPin;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = gpioPinChanged || invertedChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool GpioSwitchDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    GpioSwitchDevicePersistedConfigV1 config{};
    if (!decodeGpioSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    setSwitchConfig(config.switchConfig);
    config_ = config.gpioConfig;
    return true;
}

void GpioSwitchDevice::writeDeviceJson(JsonObject output) const {
    SwitchDeviceBase::writeDeviceJson(output);
    JsonObject configObject = output["config"].isNull() ? output.createNestedObject("config") : output["config"].as<JsonObject>();
    configObject["gpio_pin"] = config_.gpioPin;
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
    descriptor.createRuntime = &GpioSwitchDevice::createRuntime;
    descriptor.validateConfig = &GpioSwitchDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> GpioSwitchDevice::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new GpioSwitchDevice(record, configBlob));
}

DeviceValidationResult GpioSwitchDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "gpio switch config exceeds supported size"};
    }
    GpioSwitchDevicePersistedConfigV1 config{};
    if (!decodeGpioSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "gpio switch config is invalid"};
    }
    return {};
}

DeviceValidationResult GpioSwitchDevice::configureHardware(uint32_t now) {
    (void)now;
    if (!gpioSwitchPinIsValid(config_.gpioPin)) {
        return {DeviceError::InvalidConfig, "gpio switch pin is invalid"};
    }

    const OutputState initialState = startupState();
    if (initialState == OutputState::Disabled) {
        return driver_.disableOutput(config_.gpioPin) ? DeviceValidationResult{}
                                                      : DeviceValidationResult{DeviceError::StorageError, "gpio disable failed"};
    }

    const bool physicalLevel = initialState == OutputState::On ? !inverted() : inverted();
    return driver_.configureOutput(config_.gpioPin, physicalLevel)
               ? DeviceValidationResult{}
               : DeviceValidationResult{DeviceError::StorageError, "gpio configure failed"};
}

DeviceValidationResult GpioSwitchDevice::applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) {
    (void)now;
    if (state == OutputState::Disabled) {
        return driver_.disableOutput(config_.gpioPin) ? DeviceValidationResult{}
                                                      : DeviceValidationResult{DeviceError::StorageError, "gpio disable failed"};
    }

    return driver_.write(config_.gpioPin, physicalLevel) ? DeviceValidationResult{}
                                                         : DeviceValidationResult{DeviceError::StorageError, "gpio write failed"};
}

void GpioSwitchDevice::releaseHardware(uint32_t now) {
    (void)now;
    driver_.release(config_.gpioPin);
}

void GpioSwitchDevice::end(uint32_t now) {
    releaseHardware(now);
}

} // namespace ewfm
