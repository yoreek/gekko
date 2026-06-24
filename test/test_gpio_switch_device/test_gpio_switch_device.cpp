#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <cstdio>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

enum class GpioAction : uint8_t {
    Configure,
    Write,
    Disable,
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

    bool disableOutput(uint8_t pin) override {
        calls.push_back({GpioAction::Disable, pin, false});
        return disableOk;
    }

    void release(uint8_t pin) override {
        calls.push_back({GpioAction::Release, pin, false});
    }

    bool configureOk{true};
    bool writeOk{true};
    bool disableOk{true};
    std::vector<GpioCall> calls{};
};

GpioSwitchDevicePersistedConfigV1 makeGpioConfig(OutputState startup = OutputState::Off, OutputState safe = OutputState::Off) {
    GpioSwitchDevicePersistedConfigV1 config{};
    config.switchConfig.base.enabled = true;
    std::snprintf(config.switchConfig.base.name, sizeof(config.switchConfig.base.name), "%s", "relay");
    config.switchConfig.restorePreviousState = true;
    config.switchConfig.startupState = startup;
    config.switchConfig.safeState = safe;
    config.switchConfig.inverted = false;
    config.gpioConfig.gpioPin = 13;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeGpioPayload(const GpioSwitchDevicePersistedConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeGpioSwitchDeviceConfig(config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return payload;
}

void startToReady(GpioSwitchDevice& device) {
    device.begin(10);
    device.tickFastLoop(11);
}

} // namespace

void test_gpio_switch_config_round_trip() {
    GpioSwitchDevicePersistedConfigV1 config = makeGpioConfig(OutputState::On, OutputState::Disabled);
    config.switchConfig.inverted = true;
    config.gpioConfig.gpioPin = 21;

    GpioSwitchDevicePersistedConfigV1 decoded{};
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeGpioPayload(config);
    TEST_ASSERT_TRUE(decodeGpioSwitchDeviceConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL(config.switchConfig.base.enabled, decoded.switchConfig.base.enabled);
    TEST_ASSERT_EQUAL_STRING(config.switchConfig.base.name, decoded.switchConfig.base.name);
    TEST_ASSERT_EQUAL(config.switchConfig.restorePreviousState, decoded.switchConfig.restorePreviousState);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(config.switchConfig.startupState),
                            static_cast<uint8_t>(decoded.switchConfig.startupState));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(config.switchConfig.safeState), static_cast<uint8_t>(decoded.switchConfig.safeState));
    TEST_ASSERT_EQUAL(config.switchConfig.inverted, decoded.switchConfig.inverted);
    TEST_ASSERT_EQUAL_UINT8(config.gpioConfig.gpioPin, decoded.gpioConfig.gpioPin);
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
    GpioSwitchDevicePersistedConfigV1 config = makeGpioConfig(OutputState::On);
    config.switchConfig.inverted = true;
    FakeGpioOutputDriver driver;
    GpioSwitchDevice device(config, driver);

    startToReady(device);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(device.outputState()));
    TEST_ASSERT_EQUAL_UINT32(2, driver.calls.size());
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Configure), static_cast<int>(driver.calls[0].action));
    TEST_ASSERT_FALSE(driver.calls[0].level);
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Write), static_cast<int>(driver.calls[1].action));
    TEST_ASSERT_FALSE(driver.calls[1].level);
}

void test_gpio_switch_disabled_output_uses_driver_disable() {
    GpioSwitchDevicePersistedConfigV1 config = makeGpioConfig(OutputState::Disabled, OutputState::Disabled);
    FakeGpioOutputDriver driver;
    GpioSwitchDevice device(config, driver);

    startToReady(device);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Disabled), static_cast<int>(device.outputState()));
    TEST_ASSERT_EQUAL_UINT32(2, driver.calls.size());
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Disable), static_cast<int>(driver.calls[0].action));
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Disable), static_cast<int>(driver.calls[1].action));
}

void test_gpio_switch_disable_request_applies_safe_state() {
    GpioSwitchDevicePersistedConfigV1 config = makeGpioConfig(OutputState::On, OutputState::Disabled);
    FakeGpioOutputDriver driver;
    GpioSwitchDevice device(config, driver);
    startToReady(device);

    device.requestDisable();
    device.tickFastLoop(12);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Disabled), static_cast<int>(device.outputState()));
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Disable), static_cast<int>(driver.calls.back().action));
}

void test_gpio_switch_rejects_invalid_pin_config() {
    GpioSwitchDevicePersistedConfigV1 config = makeGpioConfig();
    config.gpioConfig.gpioPin = 36;

    DeviceRegistryEntry record{};
    record.header.typeId = 2;
    record.header.configVersion = 1;
    const DeviceConfigBlob configBlob = encodeGpioPayload(config);

    DeviceValidationResult result = GpioSwitchDevice::validateConfig(record, configBlob);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(result.error));
}

void test_gpio_switch_reconfigure_reapplies_current_output() {
    GpioSwitchDevicePersistedConfigV1 config = makeGpioConfig(OutputState::Off);
    FakeGpioOutputDriver driver;
    GpioSwitchDevice device(config, driver);
    startToReady(device);

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "on"}));
    device.clearRetainedStateDirty();
    device.requestReconfigure();
    device.tickFastLoop(12);
    device.tickFastLoop(13);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(device.outputState()));
    TEST_ASSERT_EQUAL(static_cast<int>(GpioAction::Write), static_cast<int>(driver.calls.back().action));
    TEST_ASSERT_TRUE(driver.calls.back().level);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gpio_switch_config_round_trip);
    RUN_TEST(test_default_device_type_registry_contains_gpio_switch);
    RUN_TEST(test_gpio_switch_startup_applies_inverted_physical_level);
    RUN_TEST(test_gpio_switch_disabled_output_uses_driver_disable);
    RUN_TEST(test_gpio_switch_disable_request_applies_safe_state);
    RUN_TEST(test_gpio_switch_rejects_invalid_pin_config);
    RUN_TEST(test_gpio_switch_reconfigure_reapplies_current_output);
    return UNITY_END();
}
