#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/oled/OledDisplayDevice.h"
#include "devices/display/oled/OledDisplayDeviceConfig.h"
#include "devices/display/oled/OledDisplayLayoutCodec.h"
#include "devices/display/oled/OledDisplayLayoutStore.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "integrations/rest/oled_display/OledDisplayDeviceApiAdapter.h"

#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

struct FixedDeviceIdSource final : public IDeviceIdSource {
    bool next(DeviceId& out) override {
        out = 77;
        return true;
    }
};

OledDisplayLayoutRecordV1 makeLayoutRecord() {
    OledDisplayLayoutRecordV1 layout{};
    layout.deviceId = 42;
    layout.recordVersion = 1;
    layout.schemaVersion = kOledDisplayLayoutSchemaVersion;
    layout.activePageIndex = 0;

    OledDisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");

    OledDisplayLayoutWidgetV1 first{};
    first.bindingKind = static_cast<uint8_t>(OledDisplayLayoutBindingKind::Metric);
    first.x = 0;
    first.y = 0;
    first.width = 64;
    first.height = 16;
    first.metricId = 7;

    OledDisplayLayoutWidgetV1 second{};
    second.bindingKind = static_cast<uint8_t>(OledDisplayLayoutBindingKind::ConstantText);
    second.x = 0;
    second.y = 16;
    second.width = 64;
    second.height = 16;
    std::snprintf(second.text, sizeof(second.text), "%s", "hello");

    page.widgets.push_back(first);
    page.widgets.push_back(second);
    layout.pages.push_back(page);
    return layout;
}

void fillOledDeviceDocument(StaticJsonDocument<1024>& doc, bool includeLayout) {
    doc.clear();
    doc["typeName"] = "oled_display";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "oled";
    config["enabled"] = true;
    config["i2cBusDeviceId"] = 12;
    config["i2cAddress"] = 0x3C;
    config["layoutWidth"] = 128;
    config["layoutHeight"] = 64;
    if (!includeLayout) {
        return;
    }
    JsonObject layout = config.createNestedObject("layout");
    layout["schemaVersion"] = 1;
    layout["activePageId"] = "main";
    JsonArray pages = layout.createNestedArray("pages");
    JsonObject page = pages.createNestedObject();
    page["id"] = "main";
    JsonArray widgets = page.createNestedArray("widgets");
    JsonObject widget = widgets.createNestedObject();
    widget["bindingKind"] = static_cast<uint8_t>(OledDisplayLayoutBindingKind::ConstantText);
    widget["x"] = 0;
    widget["y"] = 0;
    widget["width"] = 64;
    widget["height"] = 16;
    widget["text"] = "temp";
}

} // namespace

void test_oled_layout_codec_round_trip_json() {
    const OledDisplayLayoutRecordV1 original = makeLayoutRecord();
    DynamicJsonDocument doc(1024);
    JsonObject root = doc.to<JsonObject>();
    writeOledDisplayLayoutJson(original, root);

    OledDisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(parseOledDisplayLayoutJson(root, decoded));
    TEST_ASSERT_EQUAL_UINT8(original.schemaVersion, decoded.schemaVersion);
    TEST_ASSERT_EQUAL_UINT8(original.pages.size(), decoded.pages.size());
    TEST_ASSERT_EQUAL_STRING(original.pages[0].id, decoded.pages[0].id);
    TEST_ASSERT_EQUAL_UINT8(original.pages[0].widgets[1].bindingKind, decoded.pages[0].widgets[1].bindingKind);
    TEST_ASSERT_EQUAL_STRING(original.pages[0].widgets[1].text, decoded.pages[0].widgets[1].text);
    TEST_ASSERT_EQUAL_UINT8(0, decoded.activePageIndex);
}

void test_oled_layout_codec_defaults_empty_pages_to_main_page() {
    StaticJsonDocument<512> doc;
    JsonObject root = doc.to<JsonObject>();
    root["schemaVersion"] = 1;
    root["activePageId"] = "main";
    root.createNestedArray("pages");

    OledDisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(parseOledDisplayLayoutJson(root, decoded));
    TEST_ASSERT_EQUAL_UINT8(1, decoded.pages.size());
    TEST_ASSERT_EQUAL_STRING("main", decoded.pages[0].id);
    TEST_ASSERT_EQUAL_UINT8(0, decoded.activePageIndex);
}

void test_oled_layout_store_round_trip_binary() {
    MemoryConfigStorage storage;
    OledDisplayLayoutStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    const OledDisplayLayoutRecordV1 original = makeLayoutRecord();
    TEST_ASSERT_TRUE(store.save(original).ok());

    OledDisplayLayoutRecordV1 loaded{};
    TEST_ASSERT_TRUE(store.load(original.deviceId, loaded).ok());
    TEST_ASSERT_EQUAL_UINT32(original.deviceId, loaded.deviceId);
    TEST_ASSERT_EQUAL_UINT8(original.schemaVersion, loaded.schemaVersion);
    TEST_ASSERT_EQUAL_UINT8(original.pages.size(), loaded.pages.size());
    TEST_ASSERT_EQUAL_STRING(original.pages[0].id, loaded.pages[0].id);
    TEST_ASSERT_EQUAL_UINT8(original.pages[0].widgets[0].bindingKind, loaded.pages[0].widgets[0].bindingKind);
    TEST_ASSERT_EQUAL_UINT8(original.pages[0].widgets[1].bindingKind, loaded.pages[0].widgets[1].bindingKind);
    TEST_ASSERT_EQUAL_STRING(original.pages[0].widgets[1].text, loaded.pages[0].widgets[1].text);

    std::vector<uint8_t> blob;
    TEST_ASSERT_TRUE(encodeOledDisplayLayoutBinary(original, blob));
    TEST_ASSERT_TRUE(blob.size() > sizeof(OledDisplayLayoutBinaryHeaderV1));
    const auto* header = reinterpret_cast<const OledDisplayLayoutBinaryHeaderV1*>(blob.data());
    TEST_ASSERT_EQUAL_UINT16(1, header->recordVersion);
    TEST_ASSERT_EQUAL_UINT32(original.deviceId, header->deviceId);
    TEST_ASSERT_EQUAL_UINT8(1, header->pageCount);
}

void test_oled_layout_store_rejects_invalid_device_id() {
    MemoryConfigStorage storage;
    OledDisplayLayoutStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    OledDisplayLayoutRecordV1 record{};
    record.deviceId = 0;
    TEST_ASSERT_FALSE(store.save(record).ok());
}

void test_oled_layout_update_round_trip_via_registry_binary_store() {
    MemoryConfigStorage registryStorage;
    MemoryConfigStorage retainedStorage;
    MemoryConfigStorage scopedStorage;
    DeviceRegistryStore registryStore(registryStorage);
    DeviceRetainedDataStore retainedStore(retainedStorage);
    DeviceScopedDataStore scopedStore(scopedStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    TEST_ASSERT_TRUE(scopedStore.begin(false));

    FixedDeviceIdSource idSource;
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, typeRegistry, idSource, &retainedStore, &scopedStore);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    StaticJsonDocument<1024> createDoc;
    fillOledDeviceDocument(createDoc, false);
    DeviceCreateRequest createRequest{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(OledDisplayDeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), createRequest, error));
    TEST_ASSERT_NULL(error);
    const DeviceCreateResult createResult = registry.create(createRequest, 0);
    TEST_ASSERT_TRUE(createResult.ok());

    IDeviceRuntime* runtime = registry.runtime(createResult.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);

    StaticJsonDocument<1024> updateDoc;
    fillOledDeviceDocument(updateDoc, true);
    DeviceConfigUpdateRequest updateRequest{};
    TEST_ASSERT_TRUE(
        OledDisplayDeviceApiAdapter::instance().parseUpdateConfigRequest(updateDoc.as<JsonObjectConst>(), *runtime, updateRequest, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(updateRequest.persistedStateProvided);

    const DeviceMutationResult updateResult =
        registry.updateConfigAndDeps(createResult.deviceId, updateRequest.configBlob, updateRequest.configVersion,
                                     updateRequest.depsProvided, updateRequest.deps, updateRequest.depCount, 0);
    TEST_ASSERT_TRUE(updateResult.ok());
    TEST_ASSERT_TRUE(registry
                         .applyPersistedStateUpdate(createResult.deviceId, updateRequest.persistedStateBlob.data(),
                                                    updateRequest.persistedStateBlob.size())
                         .ok());
    TEST_ASSERT_TRUE(registry.flushNow().ok());

    OledDisplayLayoutStore layoutStore(scopedStore);
    OledDisplayLayoutRecordV1 storedLayout{};
    TEST_ASSERT_TRUE(layoutStore.load(createResult.deviceId, storedLayout).ok());
    TEST_ASSERT_EQUAL_UINT32(createResult.deviceId, storedLayout.deviceId);
    TEST_ASSERT_EQUAL_UINT8(1, storedLayout.pages.size());
    TEST_ASSERT_EQUAL_STRING("main", storedLayout.pages[0].id);
    TEST_ASSERT_EQUAL_STRING("temp", storedLayout.pages[0].widgets[0].text);

    DeviceRegistry reloadedRegistry(registryStore, typeRegistry, idSource, &retainedStore, &scopedStore);
    TEST_ASSERT_TRUE(reloadedRegistry.begin(0).ok());
    auto* reloadedRuntime = dynamic_cast<OledDisplayDevice*>(reloadedRegistry.runtime(createResult.deviceId));
    TEST_ASSERT_NOT_NULL(reloadedRuntime);
    TEST_ASSERT_EQUAL_UINT8(1, reloadedRuntime->layout().pages.size());
    TEST_ASSERT_EQUAL_STRING("main", reloadedRuntime->layout().pages[0].id);
    TEST_ASSERT_EQUAL_STRING("temp", reloadedRuntime->layout().pages[0].widgets[0].text);
}

void test_oled_layout_create_request_accepts_empty_pages() {
    StaticJsonDocument<1024> doc;
    fillOledDeviceDocument(doc, false);
    JsonObject config = doc["config"].as<JsonObject>();
    JsonObject layout = config.createNestedObject("layout");
    layout["activePageId"] = "main";
    layout.createNestedArray("pages");

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(OledDisplayDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(request.configBlob.size() > 0U);

    DeviceCreatePersistenceRequest persistedRequest{};
    TEST_ASSERT_TRUE(OledDisplayDeviceApiAdapter::instance().parseCreatePersistedStateRequest(doc.as<JsonObjectConst>(), request,
                                                                                              persistedRequest, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(persistedRequest.persistedStateProvided);
    OledDisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(
        decodeOledDisplayLayoutBinary(persistedRequest.persistedStateBlob.data(), persistedRequest.persistedStateBlob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(1, decoded.pages.size());
    TEST_ASSERT_EQUAL_STRING("main", decoded.pages[0].id);
}

void test_oled_layout_create_request_accepts_large_i2c_bus_device_id() {
    StaticJsonDocument<1024> doc;
    fillOledDeviceDocument(doc, true);
    JsonObject config = doc["config"].as<JsonObject>();
    config["i2cBusDeviceId"] = 4249059392UL;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(OledDisplayDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_NULL(error);

    OledDisplayDeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(
        decodeOledDisplayDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT32(4249059392UL, decoded.i2cBusDeviceId);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_oled_layout_codec_round_trip_json);
    RUN_TEST(test_oled_layout_codec_defaults_empty_pages_to_main_page);
    RUN_TEST(test_oled_layout_store_round_trip_binary);
    RUN_TEST(test_oled_layout_store_rejects_invalid_device_id);
    RUN_TEST(test_oled_layout_update_round_trip_via_registry_binary_store);
    RUN_TEST(test_oled_layout_create_request_accepts_empty_pages);
    RUN_TEST(test_oled_layout_create_request_accepts_large_i2c_bus_device_id);
    return UNITY_END();
}
