#include "devices/bus/onewire/OneWireBusDevice.h"

#include <array>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

class FakeOneWireBusDriver final : public IOneWireBusDriver {
public:
    bool begin(uint8_t pin, bool internalPullup) override {
        lastPin = pin;
        lastInternalPullup = internalPullup;
        began = true;
        return beginOk;
    }

    void depower() override {
        depowered = true;
    }

    bool reset() override {
        return true;
    }

    void resetSearch() override {
        searchIndex = 0;
    }

    bool search(OneWireRomAddress& address) override {
        if (searchIndex >= candidates.size()) {
            return false;
        }
        address = candidates[searchIndex++];
        return true;
    }

    void select(const OneWireRomAddress&) override {}

    void skip() override {}

    void write(uint8_t, bool = false) override {}

    uint8_t read() override {
        return 0;
    }

    uint8_t readBit() override {
        return 0;
    }

    uint8_t crc8(const uint8_t* data, size_t len) const override {
        uint8_t crc = 0;
        for (size_t index = 0; index < len; ++index) {
            uint8_t inbyte = data[index];
            for (uint8_t bit = 0; bit < 8; ++bit) {
                const uint8_t mix = static_cast<uint8_t>((crc ^ inbyte) & 0x01U);
                crc >>= 1U;
                if (mix != 0U) {
                    crc ^= 0x8CU;
                }
                inbyte >>= 1U;
            }
        }
        return crc;
    }

    bool beginOk{true};
    bool began{false};
    bool depowered{false};
    uint8_t lastPin{0};
    bool lastInternalPullup{false};
    size_t searchIndex{0};
    std::vector<OneWireRomAddress> candidates{};
};

OneWireRomAddress makeRom(uint8_t family, std::array<uint8_t, 6> serial, const FakeOneWireBusDriver& driver) {
    OneWireRomAddress address{};
    address.bytes[0] = family;
    for (size_t index = 0; index < serial.size(); ++index) {
        address.bytes[index + 1] = serial[index];
    }
    address.bytes[7] = driver.crc8(address.bytes, 7);
    return address;
}

OneWireBusDeviceConfigV1 makeConfig() {
    OneWireBusDeviceConfigV1 config{};
    config.enabled = 1;
    config.gpioPin = 23;
    config.internalPullup = 1;
    return config;
}

void driveToReady(OneWireBusDevice& device, uint32_t startNow = 10) {
    device.begin(startNow);
    device.tick100ms(startNow + 1U);
}

} // namespace

void test_onewire_rom_address_format_parse_and_crc() {
    FakeOneWireBusDriver driver;
    OneWireRomAddress address = makeRom(0x28, {0xFF, 0x64, 0x1D, 0x62, 0x16, 0x03}, driver);

    char formatted[17]{};
    TEST_ASSERT_TRUE(formatOneWireRomAddress(address, formatted));
    TEST_ASSERT_EQUAL_STRING("28FF641D621603AD", formatted);

    OneWireRomAddress parsed{};
    TEST_ASSERT_TRUE(parseOneWireRomAddress("28FF641D621603AD", parsed));
    TEST_ASSERT_EQUAL_MEMORY(address.bytes, parsed.bytes, sizeof(address.bytes));
    TEST_ASSERT_TRUE(oneWireRomCrcValid(driver, parsed));

    TEST_ASSERT_FALSE(parseOneWireRomAddress("bad", parsed));
    TEST_ASSERT_FALSE(parseOneWireRomAddress("28FF641D6216037Z", parsed));
}

void test_onewire_config_codec_and_json_helpers() {
    OneWireBusDeviceConfigV1 config = makeConfig();
    const std::string blob = encodeOneWireBusDeviceConfig(config);

    OneWireBusDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeOneWireBusDeviceConfig(blob, decoded));
    TEST_ASSERT_EQUAL_UINT8(config.enabled, decoded.enabled);
    TEST_ASSERT_EQUAL_UINT8(config.gpioPin, decoded.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(config.internalPullup, decoded.internalPullup);

    StaticJsonDocument<256> doc;
    JsonObject configJson = doc.to<JsonObject>();
    writeOneWireBusDeviceConfigJson(config, configJson);

    OneWireBusDeviceConfigV1 parsed{};
    std::string error;
    TEST_ASSERT_TRUE(parseOneWireBusDeviceConfigJson(configJson, parsed, error));
    TEST_ASSERT_EQUAL_UINT8(config.gpioPin, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(config.internalPullup, parsed.internalPullup);

    StaticJsonDocument<64> badDoc;
    JsonObject badJson = badDoc.to<JsonObject>();
    badJson["gpio_pin"] = "not-a-number";
    error.clear();
    TEST_ASSERT_FALSE(parseOneWireBusDeviceConfigJson(badJson, parsed, error));
    TEST_ASSERT_FALSE(error.empty());
}

void test_default_device_type_registry_contains_onewire() {
    DeviceTypeRegistry registry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = registry.find(3);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("OneWireBusDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->supportsCommands);
    TEST_ASSERT_FALSE(descriptor->supportsRetainedState);
    TEST_ASSERT_TRUE(descriptor->ticks100ms);
}

void test_onewire_runtime_scans_and_emits_state_dirty() {
    FakeOneWireBusDriver driver;
    const OneWireRomAddress valid = makeRom(0x28, {0xFF, 0x64, 0x1D, 0x62, 0x16, 0x03}, driver);
    OneWireRomAddress invalid = valid;
    invalid.bytes[7] ^= 0xFFU;
    driver.candidates = {valid, invalid, valid};

    OneWireBusDevice device(makeConfig(), driver);
    driveToReady(device);
    IDeviceRuntime* runtime = &device;

    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::Scan, 44, ""}));
    TEST_ASSERT_TRUE(device.scan().inProgress);
    TEST_ASSERT_TRUE(runtime->runtimeStateDirty());
    runtime->clearRuntimeStateDirty();
    TEST_ASSERT_FALSE(runtime->runtimeStateDirty());

    device.tick100ms(20);
    TEST_ASSERT_TRUE(device.scan().inProgress);
    TEST_ASSERT_FALSE(device.scan().ready);
    TEST_ASSERT_EQUAL_UINT8(1, device.scan().deviceCount);
    TEST_ASSERT_TRUE(runtime->runtimeStateDirty());
    runtime->clearRuntimeStateDirty();

    device.tick100ms(21);
    TEST_ASSERT_TRUE(device.scan().inProgress);
    TEST_ASSERT_FALSE(device.scan().ready);
    TEST_ASSERT_TRUE(device.scan().invalidCandidateSeen);
    TEST_ASSERT_EQUAL_UINT8(1, device.scan().deviceCount);
    TEST_ASSERT_TRUE(runtime->runtimeStateDirty());
    runtime->clearRuntimeStateDirty();

    device.tick100ms(22);
    TEST_ASSERT_TRUE(device.scan().inProgress);
    TEST_ASSERT_FALSE(device.scan().ready);
    TEST_ASSERT_EQUAL_UINT8(2, device.scan().deviceCount);
    TEST_ASSERT_TRUE(runtime->runtimeStateDirty());
    runtime->clearRuntimeStateDirty();

    device.tick100ms(23);
    TEST_ASSERT_FALSE(device.scan().inProgress);
    TEST_ASSERT_TRUE(device.scan().ready);
    TEST_ASSERT_FALSE(device.scan().truncated);
    TEST_ASSERT_EQUAL_UINT8(2, device.scan().deviceCount);
    TEST_ASSERT_TRUE(runtime->runtimeStateDirty());
    runtime->clearRuntimeStateDirty();

    char formatted[17]{};
    TEST_ASSERT_TRUE(formatOneWireRomAddress(device.scan().devices[0], formatted));
    TEST_ASSERT_EQUAL_STRING("28FF641D621603AD", formatted);
}

void test_onewire_runtime_rejects_duplicate_scan_and_faults_on_begin_failure() {
    FakeOneWireBusDriver driver;
    driver.beginOk = false;
    OneWireBusDevice failed(makeConfig(), driver);
    driveToReady(failed);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(failed.status()));

    FakeOneWireBusDriver readyDriver;
    readyDriver.candidates = {};
    OneWireBusDevice device(makeConfig(), readyDriver);
    driveToReady(device);
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::Scan, 44, ""}));
    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::Scan, 44, ""}));
}

void test_onewire_runtime_truncates_results_and_clears_on_disable_and_reconfigure() {
    FakeOneWireBusDriver driver;
    for (int index = 0; index < 18; ++index) {
        OneWireRomAddress address = makeRom(0x28, {static_cast<uint8_t>(index), 1, 2, 3, 4, 5}, driver);
        driver.candidates.push_back(address);
    }

    OneWireBusDevice device(makeConfig(), driver);
    driveToReady(device);
    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::Scan, 44, ""}));
    for (int tick = 0; tick < 19; ++tick) {
        device.tick100ms(100 + tick);
    }
    TEST_ASSERT_TRUE(device.scan().ready);
    TEST_ASSERT_TRUE(device.scan().truncated);
    TEST_ASSERT_EQUAL_UINT8(16, device.scan().deviceCount);

    device.requestDisable();
    device.tick100ms(300);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));
    TEST_ASSERT_FALSE(device.scan().inProgress);
    TEST_ASSERT_FALSE(device.scan().ready);
    TEST_ASSERT_EQUAL_UINT8(0, device.scan().deviceCount);

    device.requestReconfigure();
    device.tick100ms(301);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Reconfiguring), static_cast<int>(device.status()));
    device.tick100ms(302);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_onewire_rom_address_format_parse_and_crc);
    RUN_TEST(test_onewire_config_codec_and_json_helpers);
    RUN_TEST(test_default_device_type_registry_contains_onewire);
    RUN_TEST(test_onewire_runtime_scans_and_emits_state_dirty);
    RUN_TEST(test_onewire_runtime_rejects_duplicate_scan_and_faults_on_begin_failure);
    RUN_TEST(test_onewire_runtime_truncates_results_and_clears_on_disable_and_reconfigure);
    return UNITY_END();
}
