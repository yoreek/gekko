#include "devices/core/ConfigCodec.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <cstdio>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

enum class GpioAction : uint8_t {
    Configure,
    Write,
    Release,
};

struct GpioCall {
    GpioAction action{GpioAction::Configure};
    uint8_t pin{0};
    bool level{false};
};

class FakeGpioOutputDriver final : public IGpioOutputDriver {
public:
    bool configureOutput(uint8_t pin, bool initialLevel) override {
        calls.push_back({GpioAction::Configure, pin, initialLevel});
        return configureOk;
    }

    bool write(uint8_t pin, bool level) override {
        calls.push_back({GpioAction::Write, pin, level});
        return writeOk;
    }

    void release(uint8_t pin) override {
        calls.push_back({GpioAction::Release, pin, false});
    }

    bool configureOk{true};
    bool writeOk{true};
    std::vector<GpioCall> calls{};
};

GpioSwitchDeviceConfigV3 makeGpioConfig(bool startup = kSwitchOutputOff, bool safe = kSwitchOutputOff) {
    GpioSwitchDeviceConfigV3 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "relay");
    config.restorePreviousState = true;
    config.startupState = startup;
    config.safeState = safe;
    config.inverted = false;
    config.gpioPin = 13;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeGpioPayload(const GpioSwitchDeviceConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return payload;
}

EWFM_LEGACY_CONFIG_USE_BEGIN
BoundedBlob<kMaxDeviceConfigBytes> encodeLegacyGpioPayload(const GpioSwitchDevicePersistedConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    size_t pos = 0U;
    TEST_ASSERT_TRUE(appendFixedConfigSegment(SwitchDeviceConfigV1::kMagic, config.switchConfig, buffer, sizeof(buffer), pos));
    TEST_ASSERT_TRUE(appendFixedConfigSegment(GpioSwitchDeviceConfigV1::kMagic, config.gpioConfig, buffer, sizeof(buffer), pos));
    TEST_ASSERT_TRUE(payload.assign(buffer, pos));
    return payload;
}
EWFM_LEGACY_CONFIG_USE_END

void startToReady(GpioSwitchDevice& device) {
    device.begin(10);
    device.tickFastLoop(11);
}

} // namespace

void test_gpio_switch_config_round_trip() {
    GpioSwitchDeviceConfigV3 config = makeGpioConfig(kSwitchOutputOn, kSwitchOutputOff);
    config.inverted = true;
    config.gpioPin = 21;

    GpioSwitchDeviceConfigV3 decoded{};
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeGpioPayload(config);
    TEST_ASSERT_TRUE(decodeGpioSwitchDeviceConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL(config.enabled, decoded.enabled);
    TEST_ASSERT_EQUAL_STRING(config.name, decoded.name);
    TEST_ASSERT_EQUAL(config.restorePreviousState, decoded.restorePreviousState);
    TEST_ASSERT_TRUE(config.startupState == decoded.startupState);
    TEST_ASSERT_TRUE(config.safeState == decoded.safeState);
    TEST_ASSERT_EQUAL(config.inverted, decoded.inverted);
    TEST_ASSERT_EQUAL_UINT8(config.gpioPin, decoded.gpioPin);
}

void test_gpio_switch_config_migrates_v1_blob() {
    const GpioSwitchDeviceConfigV3 config = makeGpioConfig(kSwitchOutputOn, kSwitchOutputOff);
    EWFM_LEGACY_CONFIG_USE_BEGIN
    GpioSwitchDevicePersistedConfigV1 legacy{};
    EWFM_LEGACY_CONFIG_USE_END
    static_cast<DeviceBaseConfigV1&>(legacy.switchConfig) = config;
    legacy.switchConfig.restorePreviousState = config.restorePreviousState;
    legacy.switchConfig.startupState = 1U;
    legacy.switchConfig.safeState = 2U;
    legacy.switchConfig.inverted = config.inverted;
    legacy.gpioConfig.gpioPin = config.gpioPin;

    GpioSwitchDeviceConfigV3 decoded{};
    const BoundedBlob<kMaxDeviceConfigBytes> legacyPayload = encodeLegacyGpioPayload(legacy);
    TEST_ASSERT_TRUE(decodeGpioSwitchDeviceConfig(legacyPayload.data(), legacyPayload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(config.enabled, decoded.enabled);
    TEST_ASSERT_EQUAL_STRING(config.name, decoded.name);
    TEST_ASSERT_TRUE(config.startupState == decoded.startupState);
    TEST_ASSERT_TRUE(config.safeState == decoded.safeState);
    TEST_ASSERT_EQUAL_UINT8(config.gpioPin, decoded.gpioPin);

    uint8_t currentBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, decoded, currentBuffer, gpioSwitchDeviceConfigSize(decoded)));
    TEST_ASSERT_TRUE(gpioSwitchDeviceConfigSize(decoded) < legacyPayload.size());
}

void test_default_device_type_registry_contains_gpio_switch() {
    DeviceTypeRegistry registry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = registry.find(2);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("GpioSwitchDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->supportsCommands);
    TEST_ASSERT_TRUE(descriptor->supportsRetainedState);
}

void test_gpio_switch_startup_applies_inverted_physical_level() {
    GpioSwitchDeviceConfigV3 config = makeGpioConfig(kSwitchOutputOn);
    config.inverted = true;
    FakeGpioOutputDriver driver;
    GpioSwitchDevice device(config, driver);

    startToReady(device);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOn);
    TEST_ASSERT_EQUAL_UINT32(2, driver.calls.size());
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Configure), static_cast<int>(driver.calls[0].action));
    TEST_ASSERT_FALSE(driver.calls[0].level);
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Write), static_cast<int>(driver.calls[1].action));
    TEST_ASSERT_FALSE(driver.calls[1].level);
}

void test_gpio_switch_disable_request_applies_safe_state() {
    GpioSwitchDeviceConfigV3 config = makeGpioConfig(kSwitchOutputOn, kSwitchOutputOff);
    FakeGpioOutputDriver driver;
    GpioSwitchDevice device(config, driver);
    startToReady(device);

    device.requestDisable();
    device.tickFastLoop(12);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOff);
    TEST_ASSERT_TRUE(driver.calls.size() >= 3U);
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Write), static_cast<int>(driver.calls[driver.calls.size() - 2U].action));
    TEST_ASSERT_FALSE(driver.calls[driver.calls.size() - 2U].level);
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Release), static_cast<int>(driver.calls.back().action));
}

void test_gpio_switch_rejects_invalid_pin_config() {
    GpioSwitchDeviceConfigV3 config = makeGpioConfig();
    config.gpioPin = 36;

    DeviceRegistryEntry record{};
    record.header.typeId = 2;
    record.header.configVersion = 1;
    const DeviceConfigBlob configBlob = encodeGpioPayload(config);

    DeviceValidationResult result = GpioSwitchDevice::validateConfig(record, configBlob);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(result.error));
}

void test_gpio_switch_reconfigure_reapplies_current_output() {
    GpioSwitchDeviceConfigV3 config = makeGpioConfig(kSwitchOutputOff);
    FakeGpioOutputDriver driver;
    GpioSwitchDevice device(config, driver);
    startToReady(device);

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "true"}));
    device.clearRetainedStateDirty();
    device.requestReconfigure();
    device.tickFastLoop(12);
    device.tickFastLoop(13);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOn);
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Write), static_cast<int>(driver.calls.back().action));
    TEST_ASSERT_TRUE(driver.calls.back().level);
}
