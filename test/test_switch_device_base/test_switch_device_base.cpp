#include "devices/switch/BinarySwitchDeviceBase.h"
#include "devices/switch/TriStateSwitchDeviceBase.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <cstdio>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

struct OutputWrite {
    OutputState state{OutputState::Off};
    bool physicalLevel{false};
    uint32_t now{0};
};

class FakeTriStateSwitch : public TriStateSwitchDeviceBase {
public:
    explicit FakeTriStateSwitch(const SwitchDeviceConfigV1& config) : TriStateSwitchDeviceBase(config) {}

    uint8_t configureCalls{0};
    uint8_t releaseCalls{0};
    bool configureOk{true};
    bool applyOk{true};
    std::vector<OutputWrite> writes{};

    bool isRuntimeStateDirty() const {
        return runtimeStateDirty();
    }

private:
    DeviceValidationResult configureHardware(uint32_t now) override {
        (void)now;
        ++configureCalls;
        return configureOk ? DeviceValidationResult{} : DeviceValidationResult{DeviceError::InvalidConfig, "configure failed"};
    }

    DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) override {
        writes.push_back({state, physicalLevel, now});
        return applyOk ? DeviceValidationResult{} : DeviceValidationResult{DeviceError::StorageError, "apply failed"};
    }

    void releaseHardware(uint32_t now) override {
        (void)now;
        ++releaseCalls;
    }
};

class FakeBinarySwitch final : public BinarySwitchDeviceBase {
public:
    explicit FakeBinarySwitch(const SwitchDeviceConfigV1& config) : BinarySwitchDeviceBase(config) {}

    std::vector<OutputWrite> writes{};

private:
    DeviceValidationResult configureHardware(uint32_t now) override {
        (void)now;
        return {};
    }

    DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) override {
        writes.push_back({state, physicalLevel, now});
        return {};
    }

    void releaseHardware(uint32_t now) override {
        (void)now;
    }
};

class FakeGpioOutputDriver final : public IGpioOutputDriver {
public:
    bool configureOutput(uint8_t pin, bool initialLevel) override {
        configurePins.push_back(pin);
        configureLevels.push_back(initialLevel);
        return true;
    }

    bool write(uint8_t pin, bool level) override {
        writePins.push_back(pin);
        writeLevels.push_back(level);
        return true;
    }

    bool disableOutput(uint8_t pin) override {
        disablePins.push_back(pin);
        return true;
    }

    void release(uint8_t pin) override {
        releasePins.push_back(pin);
    }

    std::vector<uint8_t> configurePins{};
    std::vector<bool> configureLevels{};
    std::vector<uint8_t> writePins{};
    std::vector<bool> writeLevels{};
    std::vector<uint8_t> disablePins{};
    std::vector<uint8_t> releasePins{};
};

SwitchDeviceConfigV1 makeConfig(OutputState startup = OutputState::Off, OutputState safe = OutputState::Off) {
    SwitchDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "switch");
    config.restorePreviousState = false;
    config.startupState = startup;
    config.safeState = safe;
    config.inverted = false;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeSwitchPayload(const SwitchDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeSwitchDeviceConfig(config, buffer, switchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, switchDeviceConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeGpioSwitchPayload(const GpioSwitchDevicePersistedConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeGpioSwitchDeviceConfig(config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return payload;
}

GpioSwitchDevicePersistedConfigV1 makeGpioSwitchConfig(uint8_t pin, OutputState startup = OutputState::Off, bool restorePrevious = false) {
    GpioSwitchDevicePersistedConfigV1 config{};
    config.switchConfig.enabled = 1;
    std::snprintf(config.switchConfig.name, sizeof(config.switchConfig.name), "%s", "gpio-switch");
    config.switchConfig.restorePreviousState = restorePrevious;
    config.switchConfig.startupState = startup;
    config.switchConfig.safeState = OutputState::Disabled;
    config.switchConfig.inverted = false;
    config.gpioConfig.gpioPin = pin;
    return config;
}

void startToReady(SwitchDeviceBase& device) {
    device.begin(10);
    device.tickFastLoop(11);
}

} // namespace

void test_switch_config_round_trip_validates_output_states() {
    SwitchDeviceConfigV1 config = makeConfig(OutputState::On, OutputState::Disabled);
    config.restorePreviousState = true;
    config.inverted = true;

    SwitchDeviceConfigV1 decoded{};
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeSwitchPayload(config);
    TEST_ASSERT_TRUE(decodeSwitchDeviceConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(config.enabled, decoded.enabled);
    TEST_ASSERT_EQUAL_STRING(config.name, decoded.name);
    TEST_ASSERT_EQUAL_UINT8(config.restorePreviousState, decoded.restorePreviousState);
    TEST_ASSERT_EQUAL_UINT8(config.startupState, decoded.startupState);
    TEST_ASSERT_EQUAL_UINT8(config.safeState, decoded.safeState);
    TEST_ASSERT_EQUAL_UINT8(config.inverted, decoded.inverted);
}

void test_tri_state_switch_applies_startup_state_and_inversion() {
    SwitchDeviceConfigV1 config = makeConfig(OutputState::On);
    config.inverted = true;
    FakeTriStateSwitch device(config);

    startToReady(device);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(device.outputState()));
    TEST_ASSERT_FALSE(device.physicalOutputState());
    TEST_ASSERT_EQUAL_UINT8(1, device.configureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, device.writes.size());
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(device.writes[0].state));
    TEST_ASSERT_FALSE(device.writes[0].physicalLevel);
}

void test_tri_state_switch_restores_retained_output_state_without_dirty_write() {
    SwitchDeviceConfigV1 config = makeConfig(OutputState::Off);
    config.restorePreviousState = true;
    FakeTriStateSwitch device(config);

    device.applyRetainedState(OutputState::Disabled);
    startToReady(device);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Disabled), static_cast<int>(device.outputState()));
    TEST_ASSERT_FALSE(device.retainedStateDirty());
    TEST_ASSERT_EQUAL_UINT32(1, device.writes.size());
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Disabled), static_cast<int>(device.writes[0].state));
}

void test_switch_set_state_command_marks_retained_dirty_only_when_restore_enabled() {
    SwitchDeviceConfigV1 config = makeConfig(OutputState::Off);
    config.restorePreviousState = true;
    FakeTriStateSwitch device(config);
    startToReady(device);

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "on"}));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(device.outputState()));
    TEST_ASSERT_TRUE(device.retainedStateDirty());

    device.clearRetainedStateDirty();
    config.restorePreviousState = false;
    FakeTriStateSwitch noRestoreDevice(config);
    startToReady(noRestoreDevice);

    TEST_ASSERT_TRUE(noRestoreDevice.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "on"}));
    TEST_ASSERT_FALSE(noRestoreDevice.retainedStateDirty());
}

void test_switch_output_changes_mark_runtime_dirty() {
    SwitchDeviceConfigV1 config = makeConfig(OutputState::Off);
    FakeTriStateSwitch device(config);
    startToReady(device);

    TEST_ASSERT_FALSE(device.isRuntimeStateDirty());
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "on"}));
    TEST_ASSERT_TRUE(device.isRuntimeStateDirty());
}

void test_binary_switch_rejects_disabled_state_and_toggle_command() {
    SwitchDeviceConfigV1 config = makeConfig(OutputState::Off);
    FakeBinarySwitch device(config);
    startToReady(device);

    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "disabled"}));
    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "toggle"}));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(device.outputState()));
    TEST_ASSERT_EQUAL_UINT32(1, device.writes.size());
}

void test_disable_applies_safe_state_and_disabled_can_be_ready_capable_output() {
    SwitchDeviceConfigV1 config = makeConfig(OutputState::On, OutputState::Disabled);
    FakeTriStateSwitch device(config);
    startToReady(device);

    device.requestDisable();
    device.tickFastLoop(12);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Disabled), static_cast<int>(device.outputState()));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Disabled), static_cast<int>(device.writes.back().state));
}

void test_gpio_switch_update_hooks_release_old_pin_and_restart_with_startup_state() {
    FakeGpioOutputDriver driver;
    GpioSwitchDevicePersistedConfigV1 config = makeGpioSwitchConfig(4, OutputState::Off, false);
    GpioSwitchDevice device(config, driver);

    device.begin(10);
    device.tickFastLoop(11);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT32(1, driver.configurePins.size());
    TEST_ASSERT_EQUAL_UINT8(4, driver.configurePins[0]);

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "on"}));
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(device.outputState()));

    GpioSwitchDevicePersistedConfigV1 pinChange = config;
    pinChange.gpioConfig.gpioPin = 17;
    DeviceConfigUpdatePlan pinPlan = device.planConfigUpdate(encodeGpioSwitchPayload(pinChange));
    TEST_ASSERT_TRUE(pinPlan.endOldConfig);
    TEST_ASSERT_TRUE(pinPlan.resetStateMachine);
    static_cast<IDeviceRuntime&>(device).end(20);
    TEST_ASSERT_EQUAL_UINT32(1, driver.releasePins.size());
    TEST_ASSERT_EQUAL_UINT8(4, driver.releasePins[0]);
    TEST_ASSERT_TRUE(device.applyConfig(encodeGpioSwitchPayload(pinChange), 20));
    device.resetStateMachine(20);
    device.tickFastLoop(21);
    device.tickFastLoop(22);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT32(2, driver.configurePins.size());
    TEST_ASSERT_EQUAL_UINT8(17, driver.configurePins.back());
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(device.outputState()));
    TEST_ASSERT_EQUAL_UINT8(17, driver.writePins.back());

    GpioSwitchDevicePersistedConfigV1 startupChange = pinChange;
    startupChange.switchConfig.startupState = OutputState::On;
    DeviceConfigUpdatePlan startupPlan = device.planConfigUpdate(encodeGpioSwitchPayload(startupChange));
    TEST_ASSERT_FALSE(startupPlan.endOldConfig);
    TEST_ASSERT_FALSE(startupPlan.resetStateMachine);
    TEST_ASSERT_TRUE(device.applyConfig(encodeGpioSwitchPayload(startupChange), 30));
    device.tickFastLoop(31);
    TEST_ASSERT_EQUAL_UINT32(2, driver.configurePins.size());
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::Off), static_cast<int>(device.outputState()));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_switch_config_round_trip_validates_output_states);
    RUN_TEST(test_tri_state_switch_applies_startup_state_and_inversion);
    RUN_TEST(test_tri_state_switch_restores_retained_output_state_without_dirty_write);
    RUN_TEST(test_switch_set_state_command_marks_retained_dirty_only_when_restore_enabled);
    RUN_TEST(test_switch_output_changes_mark_runtime_dirty);
    RUN_TEST(test_binary_switch_rejects_disabled_state_and_toggle_command);
    RUN_TEST(test_disable_applies_safe_state_and_disabled_can_be_ready_capable_output);
    RUN_TEST(test_gpio_switch_update_hooks_release_old_pin_and_restart_with_startup_state);
    return UNITY_END();
}
