#include "config/MemoryConfigStorage.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/display/ssd1306/Ssd1306Device.h"
#include "devices/display/ssd1306/Ssd1306DeviceConfig.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "integrations/rest/i2c_bus/I2cBusDeviceApiAdapter.h"
#include "integrations/rest/ssd1306/Ssd1306DeviceApiAdapter.h"

#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

DisplayLayoutRecordV1 makeLayoutRecord() {
    DisplayLayoutRecordV1 layout{};
    layout.deviceId = 42;
    layout.recordVersion = kDisplayLayoutRecordVersion;
    layout.schemaVersion = kDisplayLayoutSchemaVersion;
    layout.activePageIndex = 0;

    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");

    DisplayLayoutWidgetV1 first{};
    first.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::Metric);
    first.x = 0;
    first.y = 0;
    first.width = 64;
    first.height = 16;
    first.metricId = 7;
    first.refreshIntervalMs = 5000;

    DisplayLayoutWidgetV1 second{};
    second.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    second.x = 0;
    second.y = 16;
    second.width = 64;
    second.height = 16;
    std::snprintf(second.text, sizeof(second.text), "%s", "hello");

    DisplayLayoutWidgetV1 third{};
    third.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Bitmap);
    std::snprintf(third.id, sizeof(third.id), "%s", "bitmap");
    third.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound);
    third.width = 2;
    third.height = 2;
    third.bitmapFormat = static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1);
    third.keepAspectRatio = 1;
    third.bitmapData = {0x00, 0x01, 0x02, 0x03};

    page.widgets.push_back(first);
    page.widgets.push_back(second);
    page.widgets.push_back(third);
    layout.pages.push_back(page);
    return layout;
}

template <size_t N> void fillSsd1306DeviceDocument(StaticJsonDocument<N>& doc, bool includeLayout) {
    doc.clear();
    doc["typeName"] = "ssd1306";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "ssd1306";
    config["enabled"] = true;
    config["i2cBusDeviceId"] = 12;
    config["i2cAddress"] = 0x3C;
    config["width"] = 128;
    config["height"] = 64;
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
    widget["id"] = "text";
    widget["type"] = "text";
    widget["bindingKind"] = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    widget["x"] = 0;
    widget["y"] = 0;
    widget["width"] = 64;
    widget["height"] = 16;
    widget["text"] = "{{system.wifi.station_ip}} {{system.time}}";
    JsonObject bitmapWidget = widgets.createNestedObject();
    bitmapWidget["id"] = "bitmap";
    bitmapWidget["type"] = "bitmap";
    bitmapWidget["bindingKind"] = static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound);
    bitmapWidget["x"] = 0;
    bitmapWidget["y"] = 16;
    bitmapWidget["width"] = 2;
    bitmapWidget["height"] = 2;
    bitmapWidget["bitmapFormat"] = "mono1";
    bitmapWidget["keepAspectRatio"] = true;
    bitmapWidget["bitmapData"] = "AAECAw==";
}

void fillI2cBusDocument(StaticJsonDocument<512>& doc, const char* name, uint8_t sdaPin, uint8_t sclPin) {
    doc.clear();
    doc["typeName"] = "i2c_bus";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = name;
    config["enabled"] = true;
    config["sdaPin"] = sdaPin;
    config["sclPin"] = sclPin;
    config["internalPullup"] = true;
    config["frequencyHz"] = 100000U;
}

} // namespace

void test_ssd1306_layout_codec_round_trip_json() {
    const DisplayLayoutRecordV1 original = makeLayoutRecord();
    DynamicJsonDocument doc(4096);
    JsonObject root = doc.to<JsonObject>();
    writeDisplayLayoutJson(original, root);

    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(parseDisplayLayoutJson(root, decoded));
    TEST_ASSERT_EQUAL_UINT8(original.schemaVersion, decoded.schemaVersion);
    TEST_ASSERT_EQUAL_UINT8(original.pages.size(), decoded.pages.size());
    TEST_ASSERT_EQUAL_STRING(original.pages[0].id, decoded.pages[0].id);
    TEST_ASSERT_EQUAL_UINT8(original.pages[0].widgets[1].bindingKind, decoded.pages[0].widgets[1].bindingKind);
    TEST_ASSERT_EQUAL_UINT16(original.pages[0].widgets[0].refreshIntervalMs, decoded.pages[0].widgets[0].refreshIntervalMs);
    TEST_ASSERT_EQUAL_STRING(original.pages[0].widgets[1].text, decoded.pages[0].widgets[1].text);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutWidgetType::Bitmap), decoded.pages[0].widgets[2].type);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1), decoded.pages[0].widgets[2].bitmapFormat);
    TEST_ASSERT_EQUAL_UINT8(1, decoded.pages[0].widgets[2].keepAspectRatio);
    TEST_ASSERT_EQUAL_UINT8(4, decoded.pages[0].widgets[2].bitmapData.size());
    TEST_ASSERT_EQUAL_UINT8(0, decoded.activePageIndex);
}

void test_ssd1306_layout_codec_defaults_empty_pages_to_main_page() {
    StaticJsonDocument<512> doc;
    JsonObject root = doc.to<JsonObject>();
    root["schemaVersion"] = 1;
    root["activePageId"] = "main";
    root.createNestedArray("pages");

    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(parseDisplayLayoutJson(root, decoded));
    TEST_ASSERT_EQUAL_UINT8(1, decoded.pages.size());
    TEST_ASSERT_EQUAL_STRING("main", decoded.pages[0].id);
    TEST_ASSERT_EQUAL_UINT8(0, decoded.activePageIndex);
}

void test_ssd1306_layout_store_round_trip_binary() {
    MemoryConfigStorage storage;
    DisplayLayoutStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    const DisplayLayoutRecordV1 original = makeLayoutRecord();
    TEST_ASSERT_TRUE(store.save(original).ok());

    DisplayLayoutRecordV1 loaded{};
    TEST_ASSERT_TRUE(store.load(original.deviceId, loaded).ok());
    TEST_ASSERT_EQUAL_UINT32(original.deviceId, loaded.deviceId);
    TEST_ASSERT_EQUAL_UINT8(original.schemaVersion, loaded.schemaVersion);
    TEST_ASSERT_EQUAL_UINT8(original.pages.size(), loaded.pages.size());
    TEST_ASSERT_EQUAL_STRING(original.pages[0].id, loaded.pages[0].id);
    TEST_ASSERT_EQUAL_UINT8(original.pages[0].widgets[0].bindingKind, loaded.pages[0].widgets[0].bindingKind);
    TEST_ASSERT_EQUAL_UINT16(original.pages[0].widgets[0].refreshIntervalMs, loaded.pages[0].widgets[0].refreshIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(original.pages[0].widgets[1].bindingKind, loaded.pages[0].widgets[1].bindingKind);
    TEST_ASSERT_EQUAL_STRING(original.pages[0].widgets[1].text, loaded.pages[0].widgets[1].text);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutWidgetType::Bitmap), loaded.pages[0].widgets[2].type);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1), loaded.pages[0].widgets[2].bitmapFormat);
    TEST_ASSERT_EQUAL_UINT8(4, loaded.pages[0].widgets[2].bitmapData.size());

    std::vector<uint8_t> blob;
    TEST_ASSERT_TRUE(encodeDisplayLayoutBinary(original, blob));
    TEST_ASSERT_TRUE(blob.size() > sizeof(DisplayLayoutBinaryHeaderV1));
    const auto* header = reinterpret_cast<const DisplayLayoutBinaryHeaderV1*>(blob.data());
    TEST_ASSERT_EQUAL_UINT16(kDisplayLayoutRecordVersion, header->recordVersion);
    TEST_ASSERT_EQUAL_UINT32(original.deviceId, header->deviceId);
    TEST_ASSERT_EQUAL_UINT8(1, header->pageCount);
}

void test_ssd1306_layout_store_rejects_invalid_device_id() {
    MemoryConfigStorage storage;
    DisplayLayoutStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DisplayLayoutRecordV1 record{};
    record.deviceId = 0;
    TEST_ASSERT_FALSE(store.save(record).ok());
}

void test_ssd1306_layout_update_round_trip_via_registry_binary_store() {
    MemoryConfigStorage registryStorage;
    MemoryConfigStorage retainedStorage;
    MemoryConfigStorage scopedStorage;
    DeviceRegistryStore registryStore(registryStorage);
    DeviceRetainedDataStore retainedStore(retainedStorage);
    DeviceScopedDataStore scopedStore(scopedStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    TEST_ASSERT_TRUE(scopedStore.begin(false));

    SequentialDeviceIdSource idSource(100);
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, typeRegistry, idSource, &retainedStore, &scopedStore);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    StaticJsonDocument<512> busDoc;
    fillI2cBusDocument(busDoc, "bus", 21, 22);
    DeviceCreateRequest busRequest{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(I2cBusDeviceApiAdapter::instance().parseCreateRequest(busDoc.as<JsonObjectConst>(), busRequest, error));
    TEST_ASSERT_NULL(error);
    const DeviceCreateResult busResult = registry.create(busRequest, 0);
    TEST_ASSERT_TRUE(busResult.ok());

    StaticJsonDocument<2048> createDoc;
    fillSsd1306DeviceDocument(createDoc, false);
    createDoc["config"]["i2cBusDeviceId"] = busResult.deviceId;
    DeviceCreateRequest createRequest{};
    TEST_ASSERT_TRUE(Ssd1306DeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), createRequest, error));
    TEST_ASSERT_NULL(error);
    const DeviceCreateResult createResult = registry.create(createRequest, 0);
    TEST_ASSERT_TRUE(createResult.ok());

    IDeviceRuntime* runtime = registry.runtime(createResult.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);

    StaticJsonDocument<2048> updateDoc;
    fillSsd1306DeviceDocument(updateDoc, true);
    updateDoc["config"]["i2cBusDeviceId"] = busResult.deviceId;
    DeviceConfigUpdateRequest updateRequest{};
    TEST_ASSERT_TRUE(
        Ssd1306DeviceApiAdapter::instance().parseUpdateConfigRequest(updateDoc.as<JsonObjectConst>(), *runtime, updateRequest, error));
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

    DisplayLayoutStore layoutStore(scopedStore);
    DisplayLayoutRecordV1 storedLayout{};
    TEST_ASSERT_TRUE(layoutStore.load(createResult.deviceId, storedLayout).ok());
    TEST_ASSERT_EQUAL_UINT32(createResult.deviceId, storedLayout.deviceId);
    TEST_ASSERT_EQUAL_UINT8(1, storedLayout.pages.size());
    TEST_ASSERT_EQUAL_STRING("main", storedLayout.pages[0].id);
    TEST_ASSERT_EQUAL_STRING("{{system.wifi.station_ip}} {{system.time}}", storedLayout.pages[0].widgets[0].text);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutWidgetType::Bitmap), storedLayout.pages[0].widgets[1].type);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1), storedLayout.pages[0].widgets[1].bitmapFormat);
    TEST_ASSERT_EQUAL_UINT8(4, storedLayout.pages[0].widgets[1].bitmapData.size());

    DeviceRegistry reloadedRegistry(registryStore, typeRegistry, idSource, &retainedStore, &scopedStore);
    TEST_ASSERT_TRUE(reloadedRegistry.begin(0).ok());
    auto* reloadedRuntime = dynamic_cast<Ssd1306Device*>(reloadedRegistry.runtime(createResult.deviceId));
    TEST_ASSERT_NOT_NULL(reloadedRuntime);
    TEST_ASSERT_EQUAL_UINT8(1, reloadedRuntime->layout().pages.size());
    TEST_ASSERT_EQUAL_STRING("main", reloadedRuntime->layout().pages[0].id);
    TEST_ASSERT_EQUAL_STRING("{{system.wifi.station_ip}} {{system.time}}", reloadedRuntime->layout().pages[0].widgets[0].text);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutWidgetType::Bitmap), reloadedRuntime->layout().pages[0].widgets[1].type);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1),
                            reloadedRuntime->layout().pages[0].widgets[1].bitmapFormat);
    TEST_ASSERT_EQUAL_UINT8(4, reloadedRuntime->layout().pages[0].widgets[1].bitmapData.size());
}

void test_ssd1306_layout_create_request_accepts_empty_pages() {
    StaticJsonDocument<2048> doc;
    fillSsd1306DeviceDocument(doc, false);
    JsonObject config = doc["config"].as<JsonObject>();
    JsonObject layout = config.createNestedObject("layout");
    layout["activePageId"] = "main";
    layout.createNestedArray("pages");

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Ssd1306DeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(request.configBlob.size() > 0U);

    DeviceCreatePersistenceRequest persistedRequest{};
    TEST_ASSERT_TRUE(
        Ssd1306DeviceApiAdapter::instance().parseCreatePersistedStateRequest(doc.as<JsonObjectConst>(), request, persistedRequest, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(persistedRequest.persistedStateProvided);
    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(
        decodeDisplayLayoutBinary(persistedRequest.persistedStateBlob.data(), persistedRequest.persistedStateBlob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(1, decoded.pages.size());
    TEST_ASSERT_EQUAL_STRING("main", decoded.pages[0].id);
}

void test_ssd1306_layout_create_request_keeps_text_placeholders() {
    StaticJsonDocument<2048> doc;
    fillSsd1306DeviceDocument(doc, true);
    JsonObject widget = doc["config"]["layout"]["pages"][0]["widgets"][0].as<JsonObject>();
    widget["text"] = "{{system.wifi.station_ip}} {{system.time}}";

    DeviceCreateRequest request{};
    DeviceCreatePersistenceRequest persistedRequest{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        Ssd1306DeviceApiAdapter::instance().parseCreatePersistedStateRequest(doc.as<JsonObjectConst>(), request, persistedRequest, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(persistedRequest.persistedStateProvided);
    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(
        decodeDisplayLayoutBinary(persistedRequest.persistedStateBlob.data(), persistedRequest.persistedStateBlob.size(), decoded));
    TEST_ASSERT_EQUAL_STRING("{{system.wifi.station_ip}} {{system.time}}", decoded.pages[0].widgets[0].text);
}

void test_ssd1306_layout_codec_accepts_legacy_numeric_binding_kind() {
    StaticJsonDocument<1024> doc;
    JsonObject root = doc.to<JsonObject>();
    root["schemaVersion"] = 1;
    root["activePageId"] = "main";
    JsonArray pages = root.createNestedArray("pages");
    JsonObject page = pages.createNestedObject();
    page["id"] = "main";
    JsonArray widgets = page.createNestedArray("widgets");
    JsonObject widget = widgets.createNestedObject();
    widget["bindingKind"] = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    widget["x"] = 0;
    widget["y"] = 0;
    widget["width"] = 8;
    widget["height"] = 8;
    widget["text"] = "legacy";

    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(parseDisplayLayoutJson(root, decoded));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText), decoded.pages[0].widgets[0].bindingKind);
    TEST_ASSERT_EQUAL_STRING("legacy", decoded.pages[0].widgets[0].text);
}

void test_ssd1306_layout_codec_rejects_invalid_bitmap_payload() {
    StaticJsonDocument<1024> doc;
    JsonObject root = doc.to<JsonObject>();
    root["schemaVersion"] = 1;
    root["activePageId"] = "main";
    JsonArray pages = root.createNestedArray("pages");
    JsonObject page = pages.createNestedObject();
    page["id"] = "main";
    JsonArray widgets = page.createNestedArray("widgets");
    JsonObject widget = widgets.createNestedObject();
    widget["id"] = "bitmap";
    widget["type"] = "bitmap";
    widget["bitmapFormat"] = "mono1";
    widget["bitmapData"] = "not-base64";

    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_FALSE(parseDisplayLayoutJson(root, decoded));
}

void test_ssd1306_layout_create_request_accepts_large_i2c_bus_device_id() {
    StaticJsonDocument<1024> doc;
    fillSsd1306DeviceDocument(doc, false);
    JsonObject config = doc["config"].as<JsonObject>();
    config["i2cBusDeviceId"] = 4249059392UL;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Ssd1306DeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_NULL(error);

    Ssd1306DeviceConfigV3 decoded{};
    TEST_ASSERT_TRUE(
        decodeSsd1306DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT32(4249059392UL, decoded.i2cBusDeviceId);
    TEST_ASSERT_EQUAL_UINT8(0U, decoded.rotation);
}

void test_ssd1306_config_rejects_legacy_layout_dimension_fields() {
    StaticJsonDocument<1024> doc;
    fillSsd1306DeviceDocument(doc, false);
    JsonObject config = doc["config"].as<JsonObject>();
    config["layoutWidth"] = 128;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(Ssd1306DeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_NOT_NULL(error);
}

void test_ssd1306_layout_rejects_duplicate_i2c_address_on_same_bus() {
    MemoryConfigStorage registryStorage;
    MemoryConfigStorage retainedStorage;
    MemoryConfigStorage scopedStorage;
    DeviceRegistryStore registryStore(registryStorage);
    DeviceRetainedDataStore retainedStore(retainedStorage);
    DeviceScopedDataStore scopedStore(scopedStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    TEST_ASSERT_TRUE(scopedStore.begin(false));

    SequentialDeviceIdSource idSource(100);
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, typeRegistry, idSource, &retainedStore, &scopedStore);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    StaticJsonDocument<512> busDoc;
    fillI2cBusDocument(busDoc, "bus", 21, 22);
    DeviceCreateRequest busRequest{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(I2cBusDeviceApiAdapter::instance().parseCreateRequest(busDoc.as<JsonObjectConst>(), busRequest, error));
    TEST_ASSERT_NULL(error);
    const DeviceCreateResult busResult = registry.create(busRequest, 0);
    TEST_ASSERT_TRUE(busResult.ok());

    StaticJsonDocument<1024> firstDoc;
    fillSsd1306DeviceDocument(firstDoc, false);
    JsonObject firstConfig = firstDoc["config"].as<JsonObject>();
    firstConfig["i2cBusDeviceId"] = busResult.deviceId;
    firstConfig["i2cAddress"] = 60;
    JsonObject firstLayout = firstConfig.createNestedObject("layout");
    firstLayout["schemaVersion"] = 1;
    firstLayout["activePageId"] = "main";
    JsonArray firstPages = firstLayout.createNestedArray("pages");
    JsonObject firstPage = firstPages.createNestedObject();
    firstPage["id"] = "main";
    firstPage.createNestedArray("widgets");
    DeviceCreateRequest firstRequest{};
    TEST_ASSERT_TRUE(Ssd1306DeviceApiAdapter::instance().parseCreateRequest(firstDoc.as<JsonObjectConst>(), firstRequest, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(registry.create(firstRequest, 0).ok());

    StaticJsonDocument<1024> secondDoc;
    fillSsd1306DeviceDocument(secondDoc, false);
    JsonObject secondConfig = secondDoc["config"].as<JsonObject>();
    secondConfig["i2cBusDeviceId"] = busResult.deviceId;
    secondConfig["i2cAddress"] = 60;
    JsonObject secondLayout = secondConfig.createNestedObject("layout");
    secondLayout["schemaVersion"] = 1;
    secondLayout["activePageId"] = "main";
    JsonArray secondPages = secondLayout.createNestedArray("pages");
    JsonObject secondPage = secondPages.createNestedObject();
    secondPage["id"] = "main";
    secondPage.createNestedArray("widgets");

    DeviceCreateRequest secondRequest{};
    TEST_ASSERT_TRUE(Ssd1306DeviceApiAdapter::instance().parseCreateRequest(secondDoc.as<JsonObjectConst>(), secondRequest, error));
    TEST_ASSERT_NULL(error);

    const DeviceValidationResult validation = Ssd1306DeviceApiAdapter::instance().validateCreateRequest(secondRequest, registry);
    TEST_ASSERT_FALSE(validation.ok());
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(DeviceError::InvalidRelationship), static_cast<uint32_t>(validation.error));
}
