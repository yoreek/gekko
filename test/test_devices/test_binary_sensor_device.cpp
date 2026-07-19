#include "devices/core/DeviceTypes.h"
#include "devices/sensors/binary/BinarySensorDevice.h"
#include "devices/sensors/binary/BinarySensorDeviceConfig.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/binary_sensor/BinarySensorDeviceApiAdapter.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeGpioInputDriver final : public IGpioInputDriver {
public:
    bool configureInput(uint8_t pin, GpioInputPullMode pullMode) override {
        configuredPin = pin;
        configuredPullMode = pullMode;
        ++configureCalls;
        return configureResult;
    }

    bool read(uint8_t pin, bool& outLevel) override {
        (void)pin;
        outLevel = level;
        return readResult;
    }

    void release(uint8_t pin) override {
        releasedPin = pin;
        ++releaseCalls;
    }

    bool level{false};
    bool configureResult{true};
    bool readResult{true};
    uint8_t configuredPin{0xFF};
    GpioInputPullMode configuredPullMode{GpioInputPullMode::None};
    uint8_t releasedPin{0xFF};
    int configureCalls{0};
    int releaseCalls{0};
};

BinarySensorDeviceConfigV1 makeBinarySensorConfig() {
    BinarySensorDeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "float switch");
    config.gpioPin = 4U;
    config.pullMode = static_cast<uint8_t>(GpioInputPullMode::PullUp);
    config.inverted = 0U;
    config.debounceMs = 50U;
    return config;
}

// runtimeStateDirty()/clearRuntimeStateDirty() are protected on DeviceRuntimeBase but public on
// the IDeviceRuntime interface - go through the interface, the same way the registry does.
bool runtimeDirty(BinarySensorDevice& device) {
    return static_cast<IDeviceRuntime&>(device).runtimeStateDirty();
}

void clearRuntimeDirty(BinarySensorDevice& device) {
    static_cast<IDeviceRuntime&>(device).clearRuntimeStateDirty();
}

void tickUntilReady(BinarySensorDevice& device, uint32_t now) {
    device.begin(now);
    for (int i = 0; i < 4 && device.status() != DeviceStatus::Ready; ++i) {
        device.tickFastLoop(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
}

} // namespace

void test_binary_sensor_config_codec_round_trip_and_validation() {
    BinarySensorDeviceConfigV1 config = makeBinarySensorConfig();
    config.gpioPin = 27U;
    config.inverted = 1U;
    config.debounceMs = 250U;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = binarySensorDeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, config, buffer, size));

    BinarySensorDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, buffer, size, decoded));
    TEST_ASSERT_EQUAL_UINT8(27U, decoded.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(GpioInputPullMode::PullUp), decoded.pullMode);
    TEST_ASSERT_EQUAL_UINT8(1U, decoded.inverted);
    TEST_ASSERT_EQUAL_UINT16(250U, decoded.debounceMs);
    TEST_ASSERT_TRUE(decoded.validate().ok());
}

void test_binary_sensor_config_rejects_flash_pins_and_pull_on_input_only_pins() {
    BinarySensorDeviceConfigV1 config = makeBinarySensorConfig();

    config.gpioPin = 6U;
    TEST_ASSERT_FALSE(config.validate().ok());
    config.gpioPin = 40U;
    TEST_ASSERT_FALSE(config.validate().ok());

    // Input-only pins 34-39 are valid, but only without internal pulls.
    config.gpioPin = 34U;
    config.pullMode = static_cast<uint8_t>(GpioInputPullMode::PullUp);
    TEST_ASSERT_FALSE(config.validate().ok());
    config.pullMode = static_cast<uint8_t>(GpioInputPullMode::None);
    TEST_ASSERT_TRUE(config.validate().ok());
}

void test_binary_sensor_type_and_api_adapter_are_registered() {
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = types.find(kBinarySensorDeviceTypeId);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("BinarySensorDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->providedRoles.contains(DeviceRole::Condition));
    TEST_ASSERT_TRUE(descriptor->dependencyRequirements.empty());
    TEST_ASSERT_TRUE(descriptor->ticksFastLoop);
    TEST_ASSERT_FALSE(descriptor->supportsRetainedState);

    DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    const IDeviceApiAdapter* adapter = adapters.find(kBinarySensorDeviceTypeId);
    TEST_ASSERT_NOT_NULL(adapter);
    TEST_ASSERT_EQUAL_STRING("binary_sensor", adapter->typeName());
}

void test_binary_sensor_first_read_seeds_level_without_debounce_wait() {
    FakeGpioInputDriver driver;
    driver.level = true;
    BinarySensorDevice device(makeBinarySensorConfig(), driver);

    tickUntilReady(device, 0U);
    TEST_ASSERT_EQUAL_UINT8(4U, driver.configuredPin);
    TEST_ASSERT_EQUAL(static_cast<int>(GpioInputPullMode::PullUp), static_cast<int>(driver.configuredPullMode));
    TEST_ASSERT_FALSE(device.hasReading());

    clearRuntimeDirty(device);
    device.tickFastLoop(1U);
    TEST_ASSERT_TRUE(device.hasReading());
    TEST_ASSERT_TRUE(device.isActive());
    TEST_ASSERT_TRUE(device.rawLevel());
    TEST_ASSERT_TRUE(runtimeDirty(device));
}

void test_binary_sensor_debounce_requires_stability_and_rejects_chatter() {
    FakeGpioInputDriver driver;
    driver.level = false;
    BinarySensorDevice device(makeBinarySensorConfig(), driver);

    tickUntilReady(device, 0U);
    device.tickFastLoop(1U);
    TEST_ASSERT_FALSE(device.isActive());
    clearRuntimeDirty(device);

    // Chatter: the level flips high but bounces back before debounceMs elapses.
    driver.level = true;
    device.tickFastLoop(10U);
    device.tickFastLoop(30U);
    driver.level = false;
    device.tickFastLoop(45U);
    driver.level = true;
    device.tickFastLoop(60U);
    TEST_ASSERT_FALSE(device.isActive());
    TEST_ASSERT_FALSE(runtimeDirty(device));

    // Now the high level stays stable for the full debounce window.
    device.tickFastLoop(100U);
    TEST_ASSERT_FALSE(device.isActive());
    device.tickFastLoop(110U);
    TEST_ASSERT_TRUE(device.isActive());
    TEST_ASSERT_TRUE(runtimeDirty(device));
}

void test_binary_sensor_inverted_flag_flips_active() {
    FakeGpioInputDriver driver;
    driver.level = true;
    BinarySensorDeviceConfigV1 config = makeBinarySensorConfig();
    config.inverted = 1U;
    BinarySensorDevice device(config, driver);

    tickUntilReady(device, 0U);
    device.tickFastLoop(1U);
    TEST_ASSERT_TRUE(device.hasReading());
    TEST_ASSERT_TRUE(device.rawLevel());
    TEST_ASSERT_FALSE(device.isActive());

    driver.level = false;
    device.tickFastLoop(10U);
    device.tickFastLoop(70U);
    TEST_ASSERT_TRUE(device.isActive());
}

void test_binary_sensor_disable_releases_pin_and_reports_inactive() {
    FakeGpioInputDriver driver;
    driver.level = true;
    BinarySensorDevice device(makeBinarySensorConfig(), driver);

    tickUntilReady(device, 0U);
    device.tickFastLoop(1U);
    TEST_ASSERT_TRUE(device.isActive());

    device.requestDisable();
    device.tickFastLoop(2U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_EQUAL_UINT8(4U, driver.releasedPin);
    TEST_ASSERT_EQUAL(1, driver.releaseCalls);
    TEST_ASSERT_FALSE(device.hasReading());
    TEST_ASSERT_FALSE(device.isActive());
}

void test_binary_sensor_faults_when_configure_fails_and_recovers_on_reconfigure() {
    FakeGpioInputDriver driver;
    driver.configureResult = false;
    BinarySensorDevice device(makeBinarySensorConfig(), driver);

    device.begin(0U);
    for (int i = 0; i < 4 && device.status() != DeviceStatus::Faulted; ++i) {
        device.tickFastLoop(0U);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(device.status()));

    driver.configureResult = true;
    device.requestReconfigure();
    device.tickFastLoop(1U);
    device.tickFastLoop(2U);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
}
