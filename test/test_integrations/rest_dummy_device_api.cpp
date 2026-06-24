#include "devices/dummy/DummyDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <unity.h>

using namespace ewfm;

void test_device_api_adapter_registry_resolves_onewire();
void test_onewire_api_adapter_parses_create_request();
void test_onewire_api_adapter_rejects_invalid_config_shape();
void test_onewire_api_adapter_serializes_runtime_scan_snapshot();
void test_onewire_api_adapter_parses_update_config_request();
void test_onewire_api_adapter_rejects_missing_update_config();

namespace {

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyPayload(const DummyDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

DeviceRegistryEntry makeDummyRecord() {
    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = 42;
    record.header.typeId = DummyDevice::descriptor().typeId;
    record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 3;
    DummyDeviceConfigV1 config{};
    record.header.payloadLength = static_cast<uint32_t>(dummyDeviceConfigSize(config));
    record.status = DeviceStatus::Ready;
    return record;
}

DummyDeviceConfigV1 makeDummyConfig() {
    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "dummy-api");
    return config;
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
    doc["config_version"] = 1;
    JsonObject config = doc.createNestedObject("config");
    config["enabled"] = true;
    config["name"] = "api-dummy";

    DeviceCreateRequest request;
    const char* error = nullptr;
    const bool ok = DummyDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT32(DummyDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("api-dummy", request.name.c_str());
    TEST_ASSERT_TRUE(request.enabled);
    TEST_ASSERT_EQUAL_UINT32(DummyDevice::descriptor().currentConfigVersion, request.configVersion);

    DummyDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(
        decodeDummyDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_TRUE(parsed.enabled);
    TEST_ASSERT_EQUAL_STRING("api-dummy", parsed.name);
}

void test_dummy_device_api_adapter_rejects_invalid_payload() {
    StaticJsonDocument<256> doc;
    doc["type"] = "dummy";
    doc["name"] = "";
    doc["config_version"] = 9;

    DeviceCreateRequest request;
    const char* error = nullptr;
    const bool ok = DummyDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error);
}

void test_dummy_device_api_adapter_serializes_record() {
    const DeviceRegistryEntry record = makeDummyRecord();
    DummyDevice runtime(record, encodeDummyPayload(makeDummyConfig()));
    runtime.begin(0);
    runtime.tickFastLoop(1);
    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();

    DummyDeviceApiAdapter::instance().writeDeviceJson(runtime, runtime.status(), output);

    TEST_ASSERT_EQUAL_UINT32(42, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("dummy", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(3, output["record"]["configRevision"].as<uint32_t>());
    TEST_ASSERT_TRUE(output["config"]["enabled"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("dummy-api", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["config"]["deps"].is<JsonArrayConst>());
    TEST_ASSERT_EQUAL_STRING("ready", output["runtime"]["status"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ready", output["runtime"]["effectiveStatus"].as<const char*>());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_device_api_adapter_registry_resolves_dummy);
    RUN_TEST(test_dummy_device_api_adapter_parses_create_request);
    RUN_TEST(test_dummy_device_api_adapter_rejects_invalid_payload);
    RUN_TEST(test_dummy_device_api_adapter_serializes_record);
    RUN_TEST(test_device_api_adapter_registry_resolves_onewire);
    RUN_TEST(test_onewire_api_adapter_parses_create_request);
    RUN_TEST(test_onewire_api_adapter_rejects_invalid_config_shape);
    RUN_TEST(test_onewire_api_adapter_serializes_runtime_scan_snapshot);
    RUN_TEST(test_onewire_api_adapter_parses_update_config_request);
    RUN_TEST(test_onewire_api_adapter_rejects_missing_update_config);
    return UNITY_END();
}
