#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/gpio_switch/GpioSwitchDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <memory>
#include <unity.h>

using namespace ewfm;

namespace {

class FakeGpioOutputDriver final : public IGpioOutputDriver {
public:
    bool configureOutput(uint8_t pin, bool level) override {
        lastPin = pin;
        lastLevel = level;
        return true;
    }

    bool write(uint8_t pin, bool level) override {
        lastPin = pin;
        lastLevel = level;
        return true;
    }

    void release(uint8_t pin) override {
        lastPin = pin;
    }

    uint8_t lastPin{0};
    bool lastLevel{false};
};

BoundedBlob<kMaxDeviceConfigBytes> encodeGpioPayload(const GpioSwitchDeviceConfigV3& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return payload;
}

DeviceRegistryEntry makeGpioSwitchRecord() {
    DeviceRegistryEntry record{};
    record.header.deviceId = 7;
    record.header.typeId = GpioSwitchDevice::descriptor().typeId;
    record.header.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 3;
    GpioSwitchDeviceConfigV3 config{};
    record.header.payloadLength = static_cast<uint32_t>(gpioSwitchDeviceConfigSize(config));
    record.status = DeviceStatus::Ready;
    return record;
}

GpioSwitchDeviceConfigV3 makeGpioSwitchConfig() {
    GpioSwitchDeviceConfigV3 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "relay");
    config.restorePreviousState = true;
    config.startupState = kSwitchOutputOn;
    config.safeState = kSwitchOutputOff;
    config.inverted = true;
    config.gpioPin = 21;
    return config;
}

std::unique_ptr<GpioSwitchDevice> makeGpioSwitchRuntime(FakeGpioOutputDriver& driver) {
    const DeviceRegistryEntry record = makeGpioSwitchRecord();
    const DeviceConfigBlob configBlob = encodeGpioPayload(makeGpioSwitchConfig());
    auto runtime = std::unique_ptr<GpioSwitchDevice>(new GpioSwitchDevice(makeGpioSwitchConfig(), driver));
    runtime->bindDeviceIdentity(record, configBlob);
    runtime->begin(0);
    runtime->tickFastLoop(1);
    return runtime;
}

} // namespace

void test_device_api_adapter_registry_resolves_gpio_switch() {
    DeviceApiAdapterRegistry registry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(registry.find(GpioSwitchDevice::descriptor().typeId));
    TEST_ASSERT_NOT_NULL(registry.findByName("gpio_switch"));
}

void test_gpio_switch_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "gpio_switch";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "relay";
    config["enabled"] = true;
    config["restorePreviousState"] = true;
    config["startupState"] = true;
    config["safeState"] = false;
    config["inverted"] = true;
    config["gpioPin"] = 21;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    const bool ok = GpioSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_TRUE_MESSAGE(ok, error);
    TEST_ASSERT_EQUAL_UINT32(GpioSwitchDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("relay", request.baseConfig.name);

    GpioSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(
        decodeGpioSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_TRUE(parsed.enabled);
    TEST_ASSERT_EQUAL_STRING("relay", parsed.name);
    TEST_ASSERT_EQUAL_STRING("relay", parsed.name);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
    TEST_ASSERT_TRUE(parsed.startupState == kSwitchOutputOn);
    TEST_ASSERT_TRUE(parsed.safeState == kSwitchOutputOff);
    TEST_ASSERT_TRUE(parsed.inverted);
    TEST_ASSERT_EQUAL_UINT8(21, parsed.gpioPin);
}

void test_gpio_switch_api_adapter_rejects_invalid_pin() {
    StaticJsonDocument<128> doc;
    doc["name"] = "bad";
    JsonObject config = doc.createNestedObject("config");
    config["gpioPin"] = 36;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    const bool ok = GpioSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_STRING("gpio switch pin is invalid", error);
}

void test_gpio_switch_api_adapter_serializes_record() {
    FakeGpioOutputDriver driver;
    auto runtime = makeGpioSwitchRuntime(driver);
    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    GpioSwitchDeviceApiAdapter::instance().writeDeviceJson(*runtime, runtime->status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    TEST_ASSERT_EQUAL_UINT32(7, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("gpio_switch", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("relay", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["config"]["enabled"].as<bool>());
    TEST_ASSERT_TRUE(output["config"]["restorePreviousState"].as<bool>());
    TEST_ASSERT_TRUE(output["config"]["startupState"].as<bool>());
    TEST_ASSERT_FALSE(output["config"]["safeState"].as<bool>());
    TEST_ASSERT_TRUE(output["config"]["inverted"].as<bool>());
    TEST_ASSERT_EQUAL_UINT8(21, output["config"]["gpioPin"].as<uint8_t>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["state"].as<bool>());
    TEST_ASSERT_FALSE(output["runtime"]["output"]["physicalLevel"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("ready", output["runtime"]["status"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ready", output["runtime"]["effectiveStatus"].as<const char*>());
}

void test_gpio_switch_api_adapter_serializes_runtime_output() {
    FakeGpioOutputDriver driver;
    auto runtime = makeGpioSwitchRuntime(driver);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    GpioSwitchDeviceApiAdapter::instance().writeDeviceJson(*runtime, runtime->status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    TEST_ASSERT_TRUE(output["config"]["startupState"].as<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["state"].as<bool>());
}

void test_gpio_switch_api_adapter_parses_update_config_request() {
    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "relay";
    config["enabled"] = false;
    config["restorePreviousState"] = true;
    config["startupState"] = false;
    config["safeState"] = false;
    config["inverted"] = true;
    config["gpioPin"] = 19;

    FakeGpioOutputDriver driver;
    auto runtime = makeGpioSwitchRuntime(driver);
    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        GpioSwitchDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), *runtime, request, error), error);
    TEST_ASSERT_EQUAL_UINT32(GpioSwitchDevice::descriptor().currentConfigVersion, request.configVersion);

    GpioSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(
        decodeGpioSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_FALSE(parsed.enabled);
    TEST_ASSERT_EQUAL_STRING("relay", parsed.name);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
    TEST_ASSERT_TRUE(parsed.startupState == kSwitchOutputOff);
    TEST_ASSERT_TRUE(parsed.safeState == kSwitchOutputOff);
    TEST_ASSERT_TRUE(parsed.inverted);
    TEST_ASSERT_EQUAL_UINT8(19, parsed.gpioPin);
    TEST_ASSERT_FALSE(request.isEnabled());
    TEST_ASSERT_EQUAL_STRING("relay", request.baseConfig.name);
}

void test_gpio_switch_api_adapter_partial_update_preserves_other_fields() {
    StaticJsonDocument<128> doc;
    JsonObject config = doc.createNestedObject("config");
    config["gpioPin"] = 19;

    FakeGpioOutputDriver driver;
    auto runtime = makeGpioSwitchRuntime(driver);
    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        GpioSwitchDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), *runtime, request, error), error);

    GpioSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(
        decodeGpioSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(19, parsed.gpioPin);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
    TEST_ASSERT_TRUE(parsed.inverted);
    TEST_ASSERT_TRUE(parsed.startupState == kSwitchOutputOn);
    TEST_ASSERT_TRUE(parsed.safeState == kSwitchOutputOff);
}

void test_gpio_switch_api_adapter_rejects_missing_update_config() {
    StaticJsonDocument<64> doc;
    FakeGpioOutputDriver driver;
    auto runtime = makeGpioSwitchRuntime(driver);
    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(GpioSwitchDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), *runtime, request, error));
    TEST_ASSERT_NOT_NULL(error);
}
