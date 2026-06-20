#include "devices/bus/onewire/OneWireBusDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/onewire_bus/OneWireBusDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <array>
#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeOneWireBusDriver final : public IOneWireBusDriver {
public:
    bool begin(uint8_t pin, bool internalPullup) override {
        lastPin = pin;
        lastInternalPullup = internalPullup;
        return true;
    }

    void depower() override {}

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

    uint8_t lastPin{0};
    bool lastInternalPullup{false};
    size_t searchIndex{0};
    std::vector<OneWireRomAddress> candidates{};
};

OneWireRomAddress makeRom(uint8_t family, const std::array<uint8_t, 6>& serial, const FakeOneWireBusDriver& driver) {
    OneWireRomAddress address{};
    address.bytes[0] = family;
    for (size_t index = 0; index < serial.size(); ++index) {
        address.bytes[index + 1] = serial[index];
    }
    address.bytes[7] = driver.crc8(address.bytes, 7);
    return address;
}

DeviceRegistryEntry makeRecord(const BoundedBlob<kMaxDeviceConfigBytes>& payload) {
    DeviceRegistryEntry record{};
    record.header.deviceId = 91;
    record.header.typeId = OneWireBusDevice::descriptor().typeId;
    record.header.configVersion = OneWireBusDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.header.payloadLength = static_cast<uint32_t>(payload.size());
    record.status = DeviceStatus::Ready;
    return record;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeOneWirePayload(const OneWireBusDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeOneWireBusDeviceConfig(config, buffer, oneWireBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, oneWireBusDeviceConfigSize(config)));
    return payload;
}

} // namespace

void test_device_api_adapter_registry_resolves_onewire() {
    DeviceApiAdapterRegistry registry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(registry.find(OneWireBusDevice::descriptor().typeId));
    TEST_ASSERT_NOT_NULL(registry.findByName("onewire_bus"));
}

void test_onewire_api_adapter_parses_create_request() {
    StaticJsonDocument<256> doc;
    doc["type_id"] = OneWireBusDevice::descriptor().typeId;
    doc["name"] = "onewire";
    doc["enabled"] = true;
    doc["persistence_policy"] = "delayed";
    JsonObject config = doc.createNestedObject("config");
    config["gpio_pin"] = 18;
    config["internal_pullup"] = true;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(OneWireBusDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(OneWireBusDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("onewire", request.name.c_str());
    TEST_ASSERT_TRUE(request.enabled);

    OneWireBusDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(
        decodeOneWireBusDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(18, parsed.gpioPin);
    TEST_ASSERT_TRUE(parsed.internalPullup != 0U);
}

void test_onewire_api_adapter_rejects_invalid_config_shape() {
    StaticJsonDocument<128> doc;
    doc["name"] = "onewire";
    JsonObject config = doc.createNestedObject("config");
    config["gpio_pin"] = "bad";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(OneWireBusDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_NOT_NULL(error);
}

void test_onewire_api_adapter_serializes_runtime_scan_snapshot() {
    FakeOneWireBusDriver driver;
    const OneWireRomAddress valid = makeRom(0x28, {0xFF, 0x64, 0x1D, 0x62, 0x16, 0x03}, driver);
    driver.candidates = {valid};
    OneWireBusDeviceConfigV1 config{};
    config.base.enabled = true;
    std::snprintf(config.base.name, sizeof(config.base.name), "%s", "onewire");
    config.gpioPin = 4;
    config.internalPullup = 0;
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodeOneWirePayload(config);
    DeviceRegistryEntry record = makeRecord(payload);
    OneWireBusDevice runtime(config, driver);
    runtime.bindDeviceIdentity(record, payload);
    runtime.begin(1);
    runtime.tick100ms(2);
    runtime.handleCommand(DeviceCommand{DeviceCommandType::Scan, 91, ""});
    runtime.tick100ms(3);
    runtime.tick100ms(4);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    OneWireBusDeviceApiAdapter::instance().writeDeviceJson(runtime, output);

    TEST_ASSERT_EQUAL_STRING("onewire_bus", output["type"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(4, output["config"]["gpio_pin"].as<uint8_t>());
    TEST_ASSERT_FALSE(output["config"]["internal_pullup"].as<bool>());
    TEST_ASSERT_TRUE(output["scan"]["ready"].as<bool>());
    TEST_ASSERT_EQUAL_UINT8(1, output["scan"]["device_count"].as<uint8_t>());
    TEST_ASSERT_EQUAL_STRING("28FF641D621603AD", output["scan"]["devices"][0]["address"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("28", output["scan"]["devices"][0]["family_code"].as<const char*>());
}

void test_onewire_api_adapter_parses_update_config_request() {
    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["enabled"] = false;
    config["gpio_pin"] = 19;
    config["internal_pullup"] = true;

    OneWireBusDeviceConfigV1 current{};
    current.base.enabled = true;
    std::snprintf(current.base.name, sizeof(current.base.name), "%s", "onewire");
    current.gpioPin = 4;
    current.internalPullup = 0;
    const DeviceConfigBlob currentBlob = encodeOneWirePayload(current);
    DeviceRegistryEntry record = makeRecord(currentBlob);
    FakeOneWireBusDriver driver;
    OneWireBusDevice runtime(current, driver);
    runtime.bindDeviceIdentity(record, currentBlob);
    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(OneWireBusDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(OneWireBusDevice::descriptor().currentConfigVersion, request.configVersion);

    OneWireBusDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(
        decodeOneWireBusDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_TRUE(parsed.base.enabled != 0U);
    TEST_ASSERT_EQUAL_STRING("onewire", parsed.base.name);
    TEST_ASSERT_EQUAL_UINT8(19, parsed.gpioPin);
    TEST_ASSERT_TRUE(parsed.internalPullup != 0U);
}

void test_onewire_api_adapter_rejects_missing_update_config() {
    StaticJsonDocument<64> doc;
    OneWireBusDeviceConfigV1 current{};
    current.base.enabled = true;
    std::snprintf(current.base.name, sizeof(current.base.name), "%s", "onewire");
    current.gpioPin = 4;
    current.internalPullup = 0;
    const DeviceConfigBlob currentBlob = encodeOneWirePayload(current);
    DeviceRegistryEntry record = makeRecord(currentBlob);
    FakeOneWireBusDriver driver;
    OneWireBusDevice runtime(current, driver);
    runtime.bindDeviceIdentity(record, currentBlob);
    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(OneWireBusDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_NOT_NULL(error);
}
