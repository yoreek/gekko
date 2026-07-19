#include "config/MemoryConfigStorage.h"
#include "devices/analog/ledc/LedcAnalogOutputDevice.h"
#include "devices/analog/ledc/LedcAnalogOutputDeviceConfig.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "integrations/common/DeviceApiAdapter.h"

#include <cstdio>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

LedcAnalogOutputDeviceConfigV1 makeConfig() {
    LedcAnalogOutputDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "ledc-output");
    config.restorePreviousState = true;
    config.startupState = percentToAnalogOutputState(25U);
    config.pin = 13U;
    config.ledcChannel = 0U;
    config.frequencyHz = 5000U;
    config.dutyBits = 12U;
    config.inverted = false;
    return config;
}

DeviceConfigBlob encodeConfig(const LedcAnalogOutputDeviceConfigV1& config) {
    DeviceConfigBlob blob{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ledcAnalogOutputDeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(LedcAnalogOutputDeviceConfigV1::kMagic, config, buffer, size));
    TEST_ASSERT_TRUE(blob.assign(buffer, size));
    return blob;
}

class FakeAnalogOutputDevice final : public AnalogOutputDeviceBase {
public:
    explicit FakeAnalogOutputDevice(const AnalogOutputDeviceConfigV1& config) : AnalogOutputDeviceBase(config), config_(config) {}

    std::vector<uint16_t> writes{};
    uint8_t releaseCalls{0U};

private:
    const AnalogOutputDeviceConfigV1& config() const override {
        return config_;
    }

    DeviceValidationResult configureHardware(uint32_t now) override {
        (void)now;
        return {};
    }

    DeviceValidationResult applyHardwareOutput(uint16_t state, uint32_t now) override {
        (void)now;
        writes.push_back(state);
        return {};
    }

    void releaseHardware(uint32_t now) override {
        (void)now;
        ++releaseCalls;
    }

    AnalogOutputDeviceConfigV1 config_{};
};

} // namespace

void test_ledc_analog_output_state_helpers_round_trip() {
    TEST_ASSERT_EQUAL_UINT16(0U, percentToAnalogOutputState(0U));
    TEST_ASSERT_EQUAL_UINT16(kAnalogOutputLevelMax, percentToAnalogOutputState(100U));
    TEST_ASSERT_EQUAL_UINT8(100U, analogOutputStateToPercent(kAnalogOutputLevelMax));
}

void test_ledc_analog_output_config_v1_round_trip_and_validation() {
    const LedcAnalogOutputDeviceConfigV1 config = makeConfig();
    const DeviceConfigBlob blob = encodeConfig(config);

    LedcAnalogOutputDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeLedcAnalogOutputDeviceConfig(blob.data(), blob.size(), decoded));
    TEST_ASSERT_TRUE(decoded.validate().ok());
    TEST_ASSERT_EQUAL_UINT8(config.pin, decoded.pin);
    TEST_ASSERT_EQUAL_UINT8(config.ledcChannel, decoded.ledcChannel);
    TEST_ASSERT_TRUE(config.startupState == decoded.startupState);
    TEST_ASSERT_TRUE(config.safeState == decoded.safeState);
    TEST_ASSERT_TRUE(decoded.restorePreviousState);

    StaticJsonDocument<256> json;
    decoded.writeJson(json.to<JsonObject>());
    TEST_ASSERT_EQUAL_UINT8(25U, json["startupState"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(0U, json["safeState"].as<uint8_t>());
}

void test_ledc_analog_output_runtime_uses_single_state() {
    LedcAnalogOutputDevice device(makeConfig());
    device.begin(10U);
    device.tickFastLoop(11U);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(25U), device.currentOutputState());
    TEST_ASSERT_TRUE(device.requestOutputState(percentToAnalogOutputState(75U), 20U));
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(75U), device.currentOutputState());
    TEST_ASSERT_TRUE(device.retainedStateDirty());
    TEST_ASSERT_FALSE(device.requestOutputState(static_cast<uint16_t>(kAnalogOutputLevelMax + 1U), 21U));
}

void test_analog_output_base_applies_inversion_and_safe_state() {
    AnalogOutputDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "analog-base");
    config.startupState = percentToAnalogOutputState(25U);
    config.safeState = percentToAnalogOutputState(10U);
    config.inverted = true;
    FakeAnalogOutputDevice device(config);

    device.begin(10U);
    device.tickFastLoop(11U);
    TEST_ASSERT_EQUAL_UINT16(kAnalogOutputLevelMax - config.startupState, device.writes.back());

    device.requestDisable();
    device.tickFastLoop(12U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(config.safeState == device.currentOutputState());
    TEST_ASSERT_EQUAL_UINT16(kAnalogOutputLevelMax - config.safeState, device.writes.back());
    TEST_ASSERT_EQUAL_UINT8(1U, device.releaseCalls);
}

void test_ledc_analog_output_disabled_device_stays_released() {
    LedcAnalogOutputDeviceConfigV1 config = makeConfig();
    config.enabled = 0U;
    LedcAnalogOutputDevice device(config);

    device.begin(10U);
    device.tickFastLoop(11U);

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_FALSE(device.requestOutputState(100U, 12U));
}

void test_ledc_analog_output_restores_retained_state() {
    const LedcAnalogOutputDeviceConfigV1 config = makeConfig();
    const DeviceConfigBlob blob = encodeConfig(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 42U;
    record.header.typeId = kLedcAnalogOutputDeviceTypeId;
    record.header.configVersion = kLedcAnalogOutputDeviceConfigVersion;

    MemoryConfigStorage storage;
    DeviceRetainedDataStore retainedStore(storage);
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    LedcAnalogOutputDevice device(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);
    TEST_ASSERT_TRUE(device.requestOutputState(percentToAnalogOutputState(70U), 20U));
    TEST_ASSERT_TRUE(device.saveRetainedState(retainedStore).ok());

    LedcAnalogOutputDevice restored(record, blob);
    TEST_ASSERT_TRUE(restored.loadRetainedState(retainedStore).ok());
    restored.begin(30U);
    restored.tickFastLoop(31U);
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(70U), restored.currentOutputState());
}

void test_ledc_analog_output_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = types.find(kLedcAnalogOutputDeviceTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("LedcAnalogOutputDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::AnalogOutput));
    TEST_ASSERT_TRUE(descriptor->supportsCommands);
    TEST_ASSERT_TRUE(descriptor->supportsRetainedState);

    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kLedcAnalogOutputDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("analog_output", adapter->typeName());
    TEST_ASSERT_EQUAL_PTR(adapter, adapters.findByName("analog_output"));
}
