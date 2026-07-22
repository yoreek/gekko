#include "../test_devices/JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/ssd1306/Ssd1306Device.h"
#include "devices/dummy/DummyDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"
#include "integrations/rest/ssd1306/Ssd1306DeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <string>
#include <unity.h>

using namespace ewfm;

void test_device_api_adapter_registry_resolves_onewire();
void test_onewire_api_adapter_parses_create_request();
void test_onewire_api_adapter_rejects_invalid_config_shape();
void test_onewire_api_adapter_serializes_runtime_scan_snapshot();
void test_onewire_api_adapter_parses_update_config_request();
void test_onewire_api_adapter_rejects_missing_update_config();
void test_ssd1306_device_api_adapter_encodes_layout_update_payload();

namespace {

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyPayload(const DummyDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(DeviceBaseConfigV1::kMagic, config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
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

DeviceRegistryEntry makeSsd1306Record() {
    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = 77;
    record.header.typeId = Ssd1306Device::descriptor().typeId;
    record.header.configVersion = Ssd1306Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.status = DeviceStatus::Ready;
    return record;
}

BoundedBlob<kMaxDeviceConfigBytes> encodeSsd1306Config() {
    Ssd1306DeviceConfigV5 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "ssd1306");
    config.i2cAddress = 0x3C;
    config.rotation = 1;
    config.width = 128;
    config.height = 64;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    BoundedBlob<kMaxDeviceConfigBytes> blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ssd1306DeviceConfigV5::kMagic, config, buffer, ssd1306DeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, ssd1306DeviceConfigSize(config)));
    return blob;
}

} // namespace

void test_device_api_adapter_registry_resolves_dummy() {
    DeviceApiAdapterRegistry registry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(registry.find(DummyDevice::descriptor().typeId));
    TEST_ASSERT_NOT_NULL(registry.findByName("dummy"));
}

void test_dummy_device_api_adapter_parses_create_request() {
    StaticJsonDocument<1024> doc;
    doc["typeName"] = "dummy";
    doc["configVersion"] = 9;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "api-dummy";
    config["enabled"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-dummy.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request;
    const char* error = nullptr;
    const bool ok = DummyDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT32(DummyDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("api-dummy", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT32(DummyDevice::descriptor().currentConfigVersion, request.configVersion);

    DummyDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(DeviceBaseConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                    request.configBlob.size(), parsed));
    TEST_ASSERT_TRUE(parsed.enabled);
    TEST_ASSERT_EQUAL_STRING("api-dummy", parsed.name);
}

void test_dummy_device_api_adapter_rejects_invalid_payload() {
    StaticJsonDocument<256> doc;
    doc["typeName"] = "dummy";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "";

    DeviceCreateRequest request;
    const char* error = nullptr;
    const bool ok = DummyDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error);
    TEST_ASSERT_FALSE(ok);
    TEST_ASSERT_NOT_NULL(error);
}

void test_dummy_device_api_adapter_parses_update_request() {
    const DeviceRegistryEntry record = makeDummyRecord();
    DummyDevice runtime(record, encodeDummyPayload(makeDummyConfig()));

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "updated-dummy";
    config["enabled"] = false;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-dummy.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(DummyDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_UINT32(DummyDevice::descriptor().currentConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("updated-dummy", request.baseConfig.name);
    TEST_ASSERT_FALSE(request.isEnabled());

    DummyDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(DeviceBaseConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                    request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_STRING("updated-dummy", parsed.name);
    TEST_ASSERT_FALSE(parsed.enabled);
}

void test_dummy_device_api_adapter_serializes_record() {
    const DeviceRegistryEntry record = makeDummyRecord();
    DummyDevice runtime(record, encodeDummyPayload(makeDummyConfig()));
    runtime.begin(0);
    runtime.tickFastLoop(1);
    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();

    DummyDeviceApiAdapter::instance().writeDeviceJson(runtime, runtime.status(), output);

    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-dummy.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(42, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("dummy", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(3, output["record"]["configRevision"].as<uint32_t>());
    TEST_ASSERT_TRUE(output["config"]["enabled"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("dummy-api", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["config"]["deps"].is<JsonArrayConst>());
    TEST_ASSERT_EQUAL_STRING("ready", output["runtime"]["status"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ready", output["runtime"]["effectiveStatus"].as<const char*>());
}

void test_device_api_adapter_dependency_json_round_trip_preserves_condition_invert() {
    DeviceRegistryEntry record = makeDummyRecord();
    record.deps[0] = DeviceDependencyLink{DeviceRole::Switch, 7U, true};
    record.deps[1] = DeviceDependencyLink{DeviceRole::Condition, 8U, true};
    record.depCount = 2U;
    DummyDevice runtime(record, encodeDummyPayload(makeDummyConfig()));

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    DummyDeviceApiAdapter::instance().writeDeviceJson(runtime, runtime.status(), output);

    const JsonArrayConst serializedDeps = output["config"]["deps"].as<JsonArrayConst>();
    TEST_ASSERT_EQUAL_UINT8(2U, serializedDeps.size());
    TEST_ASSERT_TRUE(serializedDeps[0]["invert"].isNull());
    TEST_ASSERT_TRUE(serializedDeps[1]["invert"].as<bool>());

    std::array<DeviceDependencyLink, kMaxDeviceDependencies> parsedDeps{};
    uint8_t parsedCount = 0U;
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        DummyDeviceApiAdapter::instance().parseSetDepsRequest(output["config"].as<JsonObjectConst>(), parsedDeps, parsedCount, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_EQUAL_UINT8(2U, parsedCount);
    TEST_ASSERT_FALSE(parsedDeps[0].invert);
    TEST_ASSERT_TRUE(parsedDeps[1].invert);

    StaticJsonDocument<1024> fallbackDoc;
    JsonObject fallback = fallbackDoc.to<JsonObject>();
    IDeviceApiAdapter::writeFallbackDeviceJson(runtime, runtime.status(), "unknown", fallback);
    TEST_ASSERT_TRUE(fallback["config"]["deps"][0]["invert"].isNull());
    TEST_ASSERT_TRUE(fallback["config"]["deps"][1]["invert"].as<bool>());
}

void test_ssd1306_device_api_adapter_encodes_layout_update_payload() {
    const DeviceRegistryEntry record = makeSsd1306Record();
    Ssd1306Device runtime(record, encodeSsd1306Config());

    StaticJsonDocument<1024> doc;
    JsonObject root = doc.to<JsonObject>();
    JsonObject config = root.createNestedObject("config");
    config["name"] = "ssd1306";
    config["enabled"] = true;
    config["i2cAddress"] = 0x3C;
    config["width"] = 128;
    config["height"] = 64;
    JsonObject layout = config.createNestedObject("layout");
    layout["schemaVersion"] = 1;
    layout["activePageId"] = "main";
    JsonArray pages = layout.createNestedArray("pages");
    JsonObject page = pages.createNestedObject();
    page["id"] = "main";
    JsonArray widgets = page.createNestedArray("widgets");
    JsonObject widget = widgets.createNestedObject();
    widget["bindingKind"] = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    widget["x"] = 0;
    widget["y"] = 0;
    widget["width"] = 64;
    widget["height"] = 16;
    widget["text"] = "temp";

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Ssd1306DeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(request.persistedStateProvided);
    TEST_ASSERT_TRUE(request.persistedStateBlob.size() > 0U);
    TEST_ASSERT_EQUAL_UINT8(0, runtime.layout().pages.size());

    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(decodeDisplayLayoutBinary(request.persistedStateBlob.data(), request.persistedStateBlob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT32(runtime.deviceId(), decoded.deviceId);
    TEST_ASSERT_EQUAL_UINT8(1, decoded.pages.size());
    TEST_ASSERT_EQUAL_UINT8(1, decoded.pages[0].widgets.size());
    TEST_ASSERT_EQUAL_STRING("main", decoded.pages[0].id);
    TEST_ASSERT_EQUAL_STRING("temp", decoded.pages[0].widgets[0].text);

    StaticJsonDocument<1024> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    Ssd1306DeviceApiAdapter::instance().writeDeviceJson(runtime, runtime.status(), output);
    TEST_ASSERT_EQUAL_UINT16(128, output["config"]["width"].as<uint16_t>());
    TEST_ASSERT_EQUAL_UINT16(64, output["config"]["height"].as<uint16_t>());
    // The layout is no longer embedded in the device JSON; it loads via GET /api/devices/:id/layout.
    TEST_ASSERT_TRUE(output["config"]["layout"].isNull());
}
