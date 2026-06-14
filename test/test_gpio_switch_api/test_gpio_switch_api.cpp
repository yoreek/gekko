#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/gpio_switch/GpioSwitchDeviceApiAdapter.h"

#include <ArduinoJson.h>
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

    bool disableOutput(uint8_t pin) override {
        lastPin = pin;
        disabled = true;
        return true;
    }

    void release(uint8_t pin) override {
        lastPin = pin;
    }

    uint8_t lastPin{0};
    bool lastLevel{false};
    bool disabled{false};
};

DeviceRecord makeGpioSwitchRecord() {
    GpioSwitchDeviceConfigV1 config{};
    config.enabled = true;
    config.restorePreviousState = true;
    config.startupState = static_cast<uint8_t>(OutputState::On);
    config.safeState = static_cast<uint8_t>(OutputState::Disabled);
    config.inverted = true;
    config.gpioPin = 21;

    DeviceRecord record{};
    record.header.deviceId = 7;
    record.header.typeId = GpioSwitchDevice::descriptor().typeId;
    record.header.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 3;
    record.header.payloadLength = static_cast<uint32_t>(encodeGpioSwitchDeviceConfig(config).size());
    record.name = "relay";
    record.enabled = true;
    record.status = DeviceStatus::Ready;
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    record.configPayload = encodeGpioSwitchDeviceConfig(config);
    return record;
}

} // namespace

void test_device_api_adapter_registry_resolves_gpio_switch() {
    DeviceApiAdapterRegistry registry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(registry.find(GpioSwitchDevice::descriptor().typeId));
    TEST_ASSERT_NOT_NULL(registry.findByName("gpio_switch"));
}

void test_gpio_switch_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["name"] = "relay";
    doc["enabled"] = true;
    doc["persistence_policy"] = "delayed";
    JsonObject config = doc.createNestedObject("config");
    config["restore_previous_state"] = true;
    config["startup_state"] = "on";
    config["safe_state"] = "disabled";
    config["inverted"] = true;
    config["gpio_pin"] = 21;

    DeviceCreateRequest request{};
    std::string error;
    const bool ok = GpioSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_TRUE_MESSAGE(ok, error.c_str());
    TEST_ASSERT_EQUAL_UINT32(GpioSwitchDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("relay", request.name.c_str());
    TEST_ASSERT_EQUAL(static_cast<int>(DevicePersistencePolicy::Delayed), static_cast<int>(request.persistencePolicy));

    GpioSwitchDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeGpioSwitchDeviceConfig(request.configPayload, parsed));
    TEST_ASSERT_TRUE(parsed.restorePreviousState != 0U);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OutputState::On), parsed.startupState);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(OutputState::Disabled), parsed.safeState);
    TEST_ASSERT_TRUE(parsed.inverted != 0U);
    TEST_ASSERT_EQUAL_UINT8(21, parsed.gpioPin);
}

void test_gpio_switch_api_adapter_rejects_invalid_pin() {
    StaticJsonDocument<128> doc;
    doc["name"] = "bad";
    JsonObject config = doc.createNestedObject("config");
    config["gpio_pin"] = 36;

    DeviceCreateRequest request{};
    std::string error;
    const bool ok = GpioSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_EQUAL_STRING("gpio switch pin is invalid", error.c_str());
}

void test_gpio_switch_api_adapter_serializes_record() {
    DeviceRecord record = makeGpioSwitchRecord();
    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    GpioSwitchDeviceApiAdapter::instance().writeDeviceJson(record, nullptr, output);

    TEST_ASSERT_EQUAL_UINT32(7, output["device_id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("gpio_switch", output["type"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ready", output["status"].as<const char*>());
    TEST_ASSERT_TRUE(output["retained_state_supported"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("on", output["config"]["startup_state"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("disabled", output["config"]["safe_state"].as<const char*>());
    TEST_ASSERT_TRUE(output["config"]["inverted"].as<bool>());
    TEST_ASSERT_EQUAL_UINT8(21, output["config"]["gpio_pin"].as<uint8_t>());
    TEST_ASSERT_TRUE(output["output"].isNull());
}

void test_gpio_switch_api_adapter_serializes_runtime_output() {
    DeviceRecord record = makeGpioSwitchRecord();
    GpioSwitchDeviceConfigV1 config{};
    TEST_ASSERT_TRUE(decodeGpioSwitchDeviceConfig(record.configPayload, config));
    FakeGpioOutputDriver driver;
    GpioSwitchDevice runtime(config, driver);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    GpioSwitchDeviceApiAdapter::instance().writeDeviceJson(record, &runtime, output);

    TEST_ASSERT_EQUAL_STRING("on", output["config"]["startup_state"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("on", output["output"]["state"].as<const char*>());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_device_api_adapter_registry_resolves_gpio_switch);
    RUN_TEST(test_gpio_switch_api_adapter_parses_create_request);
    RUN_TEST(test_gpio_switch_api_adapter_rejects_invalid_pin);
    RUN_TEST(test_gpio_switch_api_adapter_serializes_record);
    RUN_TEST(test_gpio_switch_api_adapter_serializes_runtime_output);
    return UNITY_END();
}
