#include "devices/switch/gpio/GpioSwitchDevice.h"

#include "devices/switch/gpio/ArduinoGpioOutputDriver.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kGpioSwitchDeviceTypeId = 2;
constexpr uint32_t kGpioSwitchDeviceConfigVersion = 1;
constexpr uint8_t kMaxEsp32OutputPin = 33;
constexpr uint8_t kFlashPinStart = 6;
constexpr uint8_t kFlashPinEnd = 11;

bool copyConfigFromBlob(const std::string& blob, GpioSwitchDeviceConfigV1& config) {
    constexpr size_t kBlobSize = sizeof(GpioSwitchDeviceConfigV1::kMagicKey) + sizeof(GpioSwitchDeviceConfigV1);
    if (blob.size() != kBlobSize) {
        return false;
    }

    uint32_t magicKey{0};
    std::memcpy(&magicKey, blob.data(), sizeof(magicKey));
    if (magicKey != GpioSwitchDeviceConfigV1::kMagicKey) {
        return false;
    }
    std::memcpy(&config, blob.data() + sizeof(magicKey), sizeof(GpioSwitchDeviceConfigV1));
    return true;
}

bool stateFromConfigByte(uint8_t value) {
    OutputState state{};
    return outputStateFromByte(value, state);
}

} // namespace

static_assert(std::is_trivially_copyable<GpioSwitchDeviceConfigV1>::value, "GpioSwitchDeviceConfigV1 must be POD");
static_assert(sizeof(GpioSwitchDeviceConfigV1) == 6, "GpioSwitchDeviceConfigV1 layout changed");
static_assert(sizeof(GpioSwitchDeviceConfigV1::kMagicKey) + sizeof(GpioSwitchDeviceConfigV1) <= kMaxDeviceConfigBytes,
              "GpioSwitchDeviceConfigV1 exceeds device config bound");

std::string encodeGpioSwitchDeviceConfig(const GpioSwitchDeviceConfigV1& config) {
    std::string blob;
    blob.resize(sizeof(GpioSwitchDeviceConfigV1::kMagicKey) + sizeof(GpioSwitchDeviceConfigV1));
    std::memcpy(blob.data(), &GpioSwitchDeviceConfigV1::kMagicKey, sizeof(GpioSwitchDeviceConfigV1::kMagicKey));
    std::memcpy(blob.data() + sizeof(GpioSwitchDeviceConfigV1::kMagicKey), &config, sizeof(GpioSwitchDeviceConfigV1));
    return blob;
}

bool decodeGpioSwitchDeviceConfig(const std::string& blob, GpioSwitchDeviceConfigV1& config) {
    if (!copyConfigFromBlob(blob, config)) {
        return false;
    }
    return stateFromConfigByte(config.startupState) && stateFromConfigByte(config.safeState);
}

bool parseGpioSwitchDeviceConfigJson(const JsonObjectConst& input, GpioSwitchDeviceConfigV1& config, std::string& error) {
    auto parseState = [&error](const char* value, OutputState& state) {
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
    };

    config.enabled = (input["enabled"] | true) ? 1U : 0U;
    config.restorePreviousState = (input["restore_previous_state"] | false) ? 1U : 0U;
    config.inverted = (input["inverted"] | false) ? 1U : 0U;
    config.gpioPin = static_cast<uint8_t>(input["gpio_pin"] | static_cast<int>(config.gpioPin));

    OutputState startup{};
    OutputState safe{};
    if (!parseState(input["startup_state"] | "off", startup) || !parseState(input["safe_state"] | "off", safe)) {
        return false;
    }
    config.startupState = static_cast<uint8_t>(startup);
    config.safeState = static_cast<uint8_t>(safe);

    if (!gpioSwitchPinIsValid(config.gpioPin)) {
        error = "gpio switch pin is invalid";
        return false;
    }
    return true;
}

void writeGpioSwitchDeviceConfigJson(const GpioSwitchDeviceConfigV1& config, JsonObject output) {
    OutputState startup{};
    OutputState safe{};
    (void)outputStateFromByte(config.startupState, startup);
    (void)outputStateFromByte(config.safeState, safe);
    output["enabled"] = config.enabled != 0U;
    output["restore_previous_state"] = config.restorePreviousState != 0U;
    output["startup_state"] = outputStateName(startup);
    output["safe_state"] = outputStateName(safe);
    output["inverted"] = config.inverted != 0U;
    output["gpio_pin"] = config.gpioPin;
}

SwitchDeviceConfigV1 toSwitchDeviceConfig(const GpioSwitchDeviceConfigV1& config) {
    SwitchDeviceConfigV1 switchConfig{};
    switchConfig.enabled = config.enabled;
    switchConfig.restorePreviousState = config.restorePreviousState;
    switchConfig.startupState = config.startupState;
    switchConfig.safeState = config.safeState;
    switchConfig.inverted = config.inverted;
    return switchConfig;
}

bool gpioSwitchPinIsValid(uint8_t pin) {
    if (pin > kMaxEsp32OutputPin) {
        return false;
    }
    return pin < kFlashPinStart || pin > kFlashPinEnd;
}

GpioSwitchDevice::GpioSwitchDevice(const DeviceRecord& record)
    : GpioSwitchDevice(
          [&record]() {
              GpioSwitchDeviceConfigV1 config{};
              (void)decodeGpioSwitchDeviceConfig(record.configPayload, config);
              config.enabled = record.enabled ? 1U : 0U;
              return config;
          }(),
          defaultArduinoGpioOutputDriver()) {}

GpioSwitchDevice::GpioSwitchDevice(const GpioSwitchDeviceConfigV1& config, IGpioOutputDriver& driver)
    : TriStateSwitchDeviceBase(toSwitchDeviceConfig(config)), config_(config), driver_(driver) {}

uint8_t GpioSwitchDevice::gpioPin() const {
    return config_.gpioPin;
}

const GpioSwitchDeviceConfigV1& GpioSwitchDevice::gpioConfig() const {
    return config_;
}

DeviceTypeDescriptor GpioSwitchDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kGpioSwitchDeviceTypeId;
    descriptor.name = "GpioSwitchDevice";
    descriptor.currentConfigVersion = kGpioSwitchDeviceConfigVersion;
    descriptor.canHaveChildren = false;
    descriptor.maxChildren = 0;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.createRuntime = &GpioSwitchDevice::createRuntime;
    descriptor.validateConfig = &GpioSwitchDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> GpioSwitchDevice::createRuntime(const DeviceRecord& record) {
    return std::unique_ptr<IDeviceRuntime>(new GpioSwitchDevice(record));
}

DeviceValidationResult GpioSwitchDevice::validateConfig(const DeviceRecord& record) {
    if (record.configPayload.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "gpio switch config exceeds supported size"};
    }

    GpioSwitchDeviceConfigV1 config{};
    if (!decodeGpioSwitchDeviceConfig(record.configPayload, config)) {
        return {DeviceError::InvalidConfig, "gpio switch config is invalid"};
    }
    if (!gpioSwitchPinIsValid(config.gpioPin)) {
        return {DeviceError::InvalidConfig, "gpio switch pin is invalid"};
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

} // namespace ewfm
