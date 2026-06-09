#include "devices/dummy/DummyDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <unity.h>

using namespace ewfm;

namespace {

DeviceRecord makeDummyRecord() {
    DummyDeviceConfigV2 config{};
    config.enabled = true;
    config.restorePreviousState = true;
    config.defaultOutput = false;
    config.currentOutput = true;
    config.inverted = true;

    DeviceRecord record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = 42;
    record.header.typeId = DummyDevice::descriptor().typeId;
    record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 3;
    record.header.payloadLength = static_cast<uint32_t>(encodeDummyDeviceConfig(config).size());
    record.name = "dummy-api";
    record.enabled = true;
    record.status = DeviceStatus::Ready;
    record.configPayload = encodeDummyDeviceConfig(config);
    return record;
}

} // namespace

void test_device_api_adapter_registry_resolves_dummy() {
    DeviceApiAdapterRegistry registry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(registry.find(DummyDevice::descriptor().typeId));
    TEST_ASSERT_NOT_NULL(registry.findByName("dummy"));
}

void test_dummy_device_api_adapter_parses_create_request() {
    StaticJsonDocument<1024> doc;
    doc["type"] = "dummy";
    doc["name"] = "api-dummy";
    doc["enabled"] = true;
    doc["config_version"] = 2;
    JsonObject config = doc.createNestedObject("config");
    config["restore_previous_state"] = true;
    config["default_output"] = false;
    config["current_output"] = true;
    config["inverted"] = true;

    DeviceCreateRequest request;
    std::string error;
    const bool ok = DummyDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT32(DummyDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("api-dummy", request.name.c_str());
    TEST_ASSERT_TRUE(request.enabled);
    TEST_ASSERT_EQUAL_UINT32(DummyDevice::descriptor().currentConfigVersion, request.configVersion);

    DummyDeviceConfigV2 parsed{};
    TEST_ASSERT_TRUE(decodeDummyDeviceConfig(request.configPayload, parsed));
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
    TEST_ASSERT_TRUE(parsed.currentOutput);
    TEST_ASSERT_TRUE(parsed.inverted);
}

void test_dummy_device_api_adapter_serializes_record() {
    const DeviceRecord record = makeDummyRecord();
    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();

    DummyDeviceApiAdapter::instance().writeDeviceJson(record, output);

    TEST_ASSERT_EQUAL_UINT32(42, output["device_id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("dummy", output["type"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("dummy-api", output["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["enabled"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("ready", output["status"].as<const char*>());
    TEST_ASSERT_TRUE(output["retained_state_supported"].as<bool>());
    TEST_ASSERT_TRUE(output["retained_startup_enabled"].as<bool>());
    TEST_ASSERT_FALSE(output["retained_startup_fallback_output"].as<bool>());
    TEST_ASSERT_FALSE(output["retained_state_in_config_payload"].as<bool>());

    JsonObject config = output["config"].as<JsonObject>();
    TEST_ASSERT_TRUE(config["restore_previous_state"].as<bool>());
    TEST_ASSERT_TRUE(config["current_output"].as<bool>());
    TEST_ASSERT_TRUE(config["inverted"].as<bool>());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_device_api_adapter_registry_resolves_dummy);
    RUN_TEST(test_dummy_device_api_adapter_parses_create_request);
    RUN_TEST(test_dummy_device_api_adapter_serializes_record);
    return UNITY_END();
}
