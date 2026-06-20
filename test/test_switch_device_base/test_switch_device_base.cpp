#include "devices/switch/BinarySwitchDeviceBase.h"
#include "devices/switch/TriStateSwitchDeviceBase.h"

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

SwitchDeviceConfigV1 makeConfig(OutputState startup = OutputState::Off, OutputState safe = OutputState::Off) {
    SwitchDeviceConfigV1 config{};
    config.base.enabled = true;
    std::snprintf(config.base.name, sizeof(config.base.name), "%s", "switch");
    config.restorePreviousState = false;
    config.startupState = static_cast<uint8_t>(startup);
    config.safeState = static_cast<uint8_t>(safe);
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
    TEST_ASSERT_EQUAL_UINT8(config.base.enabled, decoded.base.enabled);
    TEST_ASSERT_EQUAL_STRING(config.base.name, decoded.base.name);
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

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_switch_config_round_trip_validates_output_states);
    RUN_TEST(test_tri_state_switch_applies_startup_state_and_inversion);
    RUN_TEST(test_tri_state_switch_restores_retained_output_state_without_dirty_write);
    RUN_TEST(test_switch_set_state_command_marks_retained_dirty_only_when_restore_enabled);
    RUN_TEST(test_switch_output_changes_mark_runtime_dirty);
    RUN_TEST(test_binary_switch_rejects_disabled_state_and_toggle_command);
    RUN_TEST(test_disable_applies_safe_state_and_disabled_can_be_ready_capable_output);
    return UNITY_END();
}
