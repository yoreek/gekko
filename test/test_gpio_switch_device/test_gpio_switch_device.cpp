#include "devices/switch/gpio/GpioSwitchDevice.h"

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

GpioSwitchDeviceConfigV1 makeGpioConfig(OutputState startup = OutputState::Off, OutputState safe = OutputState::Off) {
    GpioSwitchDeviceConfigV1 config{};
    config.enabled = true;
    config.restorePreviousState = true;
    config.startupState = static_cast<uint8_t>(startup);
    config.safeState = static_cast<uint8_t>(safe);
    config.inverted = false;
    config.gpioPin = 13;
    return config;
}

void startToReady(GpioSwitchDevice& device) {
    device.begin(10);
    device.tickFastLoop(11);
}

} // namespace

void test_gpio_switch_config_round_trip() {
    GpioSwitchDeviceConfigV1 config = makeGpioConfig(OutputState::On, OutputState::Disabled);
    config.inverted = true;
    config.gpioPin = 21;

    GpioSwitchDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeGpioSwitchDeviceConfig(encodeGpioSwitchDeviceConfig(config), decoded));
    TEST_ASSERT_EQUAL_UINT8(config.enabled, decoded.enabled);
    TEST_ASSERT_EQUAL_UINT8(config.restorePreviousState, decoded.restorePreviousState);
    TEST_ASSERT_EQUAL_UINT8(config.startupState, decoded.startupState);
    TEST_ASSERT_EQUAL_UINT8(config.safeState, decoded.safeState);
    TEST_ASSERT_EQUAL_UINT8(config.inverted, decoded.inverted);
    TEST_ASSERT_EQUAL_UINT8(config.gpioPin, decoded.gpioPin);
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
    GpioSwitchDeviceConfigV1 config = makeGpioConfig(OutputState::On);
    config.inverted = true;
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
    GpioSwitchDeviceConfigV1 config = makeGpioConfig(OutputState::Disabled, OutputState::Disabled);
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
    GpioSwitchDeviceConfigV1 config = makeGpioConfig(OutputState::On, OutputState::Disabled);
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
    GpioSwitchDeviceConfigV1 config = makeGpioConfig();
    config.gpioPin = 36;

    DeviceRecord record{};
    record.header.typeId = 2;
    record.header.configVersion = 1;
    record.configPayload = encodeGpioSwitchDeviceConfig(config);

    DeviceValidationResult result = GpioSwitchDevice::validateConfig(record);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(result.error));
}

void test_gpio_switch_reconfigure_reapplies_current_output() {
    GpioSwitchDeviceConfigV1 config = makeGpioConfig(OutputState::Off);
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
