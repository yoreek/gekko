#include "config/MemoryConfigStorage.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/switch/SwitchDeviceBase.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"

#include <cstdio>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

struct OutputWrite {
    bool state{kSwitchOutputOff};
    bool physicalLevel{false};
    uint32_t now{0};
};

struct LegacySwitchRetainedStateRecord {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    DeviceId deviceId{0};
    uint8_t outputState{0U};
    uint8_t reserved[3]{};
};

static_assert(sizeof(LegacySwitchRetainedStateRecord) == sizeof(BoolOutputDeviceRetainedStateRecord),
              "legacy switch retained state test layout changed");

class FakeSwitch : public SwitchDeviceBase {
public:
    explicit FakeSwitch(const SwitchDeviceConfigV2& config) : SwitchDeviceBase(config), config_(config) {}

    uint8_t configureCalls{0};
    uint8_t releaseCalls{0};
    bool configureOk{true};
    bool applyOk{true};
    std::vector<OutputWrite> writes{};

    bool isRuntimeStateDirty() const {
        return runtimeStateDirty();
    }

private:
    const SwitchDeviceConfigV2& config() const override {
        return config_;
    }

    SwitchDeviceConfigV2& mutableConfig() override {
        return config_;
    }

    DeviceValidationResult configureHardware(uint32_t now) override {
        (void)now;
        ++configureCalls;
        return configureOk ? DeviceValidationResult{} : DeviceValidationResult{DeviceError::InvalidConfig, "configure failed"};
    }

    DeviceValidationResult applyHardwareOutput(bool physicalLevel, uint32_t now) override {
        const bool state = physicalLevel == config_.inverted ? kSwitchOutputOff : kSwitchOutputOn;
        writes.push_back({state, physicalLevel, now});
        return applyOk ? DeviceValidationResult{} : DeviceValidationResult{DeviceError::StorageError, "apply failed"};
    }

    void releaseHardware(uint32_t now) override {
        (void)now;
        ++releaseCalls;
    }

    SwitchDeviceConfigV2 config_{};
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

    void release(uint8_t pin) override {
        releasePins.push_back(pin);
    }

    std::vector<uint8_t> configurePins{};
    std::vector<bool> configureLevels{};
    std::vector<uint8_t> writePins{};
    std::vector<bool> writeLevels{};
    std::vector<uint8_t> releasePins{};
};

SwitchDeviceConfigV2 makeConfig(bool startup = kSwitchOutputOff, bool safe = kSwitchOutputOff) {
    SwitchDeviceConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "switch");
    config.restorePreviousState = false;
    config.startupState = startup;
    config.safeState = safe;
    config.inverted = false;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeSwitchPayload(const SwitchDeviceConfigV2& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(SwitchDeviceConfigV2::kMagic, config, buffer, switchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, switchDeviceConfigSize(config)));
    return payload;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeGpioSwitchPayload(const GpioSwitchDeviceConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return payload;
}

GpioSwitchDeviceConfigV3 makeGpioSwitchConfig(uint8_t pin, bool startup = kSwitchOutputOff, bool restorePrevious = false) {
    GpioSwitchDeviceConfigV3 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", "gpio-switch");
    config.restorePreviousState = restorePrevious;
    config.startupState = startup;
    config.safeState = kSwitchOutputOff;
    config.inverted = false;
    config.gpioPin = pin;
    return config;
}

void startToReady(SwitchDeviceBase& device) {
    device.begin(10);
    device.tickFastLoop(11);
}

} // namespace

void test_switch_config_round_trip_validates_output_states() {
    SwitchDeviceConfigV2 config = makeConfig(kSwitchOutputOn, kSwitchOutputOff);
    config.restorePreviousState = true;
    config.inverted = true;

    SwitchDeviceConfigV2 decoded{};
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeSwitchPayload(config);
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(SwitchDeviceConfigV2::kMagic, payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(config.enabled, decoded.enabled);
    TEST_ASSERT_EQUAL_STRING(config.name, decoded.name);
    TEST_ASSERT_EQUAL_UINT8(config.restorePreviousState, decoded.restorePreviousState);
    TEST_ASSERT_TRUE(config.startupState == decoded.startupState);
    TEST_ASSERT_TRUE(config.safeState == decoded.safeState);
    TEST_ASSERT_EQUAL_UINT8(config.inverted, decoded.inverted);
}

void test_switch_applies_startup_state_and_inversion() {
    SwitchDeviceConfigV2 config = makeConfig(kSwitchOutputOn);
    config.inverted = true;
    FakeSwitch device(config);

    startToReady(device);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOn);
    TEST_ASSERT_FALSE(device.physicalOutputState());
    TEST_ASSERT_EQUAL_UINT8(1, device.configureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, device.writes.size());
    TEST_ASSERT_TRUE(device.writes[0].state == kSwitchOutputOn);
    TEST_ASSERT_FALSE(device.writes[0].physicalLevel);
}

void test_switch_restores_retained_output_state_without_dirty_write() {
    SwitchDeviceConfigV2 config = makeConfig(kSwitchOutputOff);
    config.restorePreviousState = true;
    FakeSwitch device(config);

    device.applyRetainedState(kSwitchOutputOn);
    startToReady(device);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOn);
    TEST_ASSERT_FALSE(device.retainedStateDirty());
    TEST_ASSERT_EQUAL_UINT32(1, device.writes.size());
    TEST_ASSERT_TRUE(device.writes[0].state == kSwitchOutputOn);
}

void test_switch_migrates_legacy_disabled_retained_state() {
    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    LegacySwitchRetainedStateRecord legacy{};
    legacy.deviceId = 51U;
    legacy.outputState = 2U;
    TEST_ASSERT_TRUE(retainedStore.save(legacy).ok());

    SwitchDeviceConfigV2 config = makeConfig(kSwitchOutputOff);
    config.restorePreviousState = true;
    FakeSwitch device(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = legacy.deviceId;
    device.bindDeviceIdentity(record, encodeSwitchPayload(config));

    TEST_ASSERT_TRUE(device.loadRetainedState(retainedStore).ok());
    startToReady(device);
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOff);
}

void test_switch_set_state_command_marks_retained_dirty_only_when_restore_enabled() {
    SwitchDeviceConfigV2 config = makeConfig(kSwitchOutputOff);
    config.restorePreviousState = true;
    FakeSwitch device(config);
    startToReady(device);

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "true"}));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOn);
    TEST_ASSERT_TRUE(device.retainedStateDirty());

    device.clearRetainedStateDirty();
    config.restorePreviousState = false;
    FakeSwitch noRestoreDevice(config);
    startToReady(noRestoreDevice);

    TEST_ASSERT_TRUE(noRestoreDevice.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "true"}));
    TEST_ASSERT_FALSE(noRestoreDevice.retainedStateDirty());
}

void test_switch_output_changes_mark_runtime_dirty() {
    SwitchDeviceConfigV2 config = makeConfig(kSwitchOutputOff);
    FakeSwitch device(config);
    startToReady(device);

    TEST_ASSERT_FALSE(device.isRuntimeStateDirty());
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "true"}));
    TEST_ASSERT_TRUE(device.isRuntimeStateDirty());
}

void test_switch_rejects_non_boolean_output_commands() {
    SwitchDeviceConfigV2 config = makeConfig(kSwitchOutputOff);
    FakeSwitch device(config);
    startToReady(device);

    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "\"disabled\""}));
    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "toggle"}));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOff);
    TEST_ASSERT_EQUAL_UINT32(1, device.writes.size());
}

void test_disable_applies_safe_state_and_releases_output() {
    SwitchDeviceConfigV2 config = makeConfig(kSwitchOutputOn, kSwitchOutputOff);
    FakeSwitch device(config);
    startToReady(device);

    device.requestDisable();
    device.tickFastLoop(12);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOff);
    TEST_ASSERT_TRUE(device.writes.back().state == kSwitchOutputOff);
    TEST_ASSERT_EQUAL_UINT8(1, device.releaseCalls);
}

void test_gpio_switch_update_hooks_release_old_pin_and_restart_with_startup_state() {
    FakeGpioOutputDriver driver;
    GpioSwitchDeviceConfigV3 config = makeGpioSwitchConfig(4, kSwitchOutputOff, false);
    GpioSwitchDevice device(config, driver);

    device.begin(10);
    device.tickFastLoop(11);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT32(1, driver.configurePins.size());
    TEST_ASSERT_EQUAL_UINT8(4, driver.configurePins[0]);

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::SetOutput, 1, "true"}));
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOn);

    GpioSwitchDeviceConfigV3 pinChange = config;
    pinChange.gpioPin = 17;
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
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOff);
    TEST_ASSERT_EQUAL_UINT8(17, driver.writePins.back());

    GpioSwitchDeviceConfigV3 startupChange = pinChange;
    startupChange.startupState = kSwitchOutputOn;
    DeviceConfigUpdatePlan startupPlan = device.planConfigUpdate(encodeGpioSwitchPayload(startupChange));
    TEST_ASSERT_FALSE(startupPlan.endOldConfig);
    TEST_ASSERT_FALSE(startupPlan.resetStateMachine);
    TEST_ASSERT_TRUE(device.applyConfig(encodeGpioSwitchPayload(startupChange), 30));
    device.tickFastLoop(31);
    TEST_ASSERT_EQUAL_UINT32(2, driver.configurePins.size());
    TEST_ASSERT_TRUE(device.currentOutputState() == kSwitchOutputOff);
}
