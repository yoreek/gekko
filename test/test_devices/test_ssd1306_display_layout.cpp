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

#include <cstring>
#include <memory>
#include <string>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

class StringJsonChunkSink final : public IJsonChunkSink {
public:
    bool emit(const char* data, const size_t size) override {
        output.append(data, size);
        return true;
    }

    bool emitJson(JsonDocument& document, const bool leadingComma) override {
        if (leadingComma) {
            output.push_back(',');
        }
        serializeJson(document, output);
        return true;
    }

    std::string output{};
};

DisplayLayoutRecordV1 makeLayoutRecord() {
    DisplayLayoutRecordV1 layout{};
    layout.deviceId = 42;
    layout.recordVersion = kDisplayLayoutRecordVersion;
    layout.schemaVersion = kDisplayLayoutSchemaVersion;
    layout.activePageIndex = 0;
    layout.backgroundColor = 0xF800U;

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
    first.color = 0x07E0U;

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
    config["i2cAddress"] = 0x3C;
    config["width"] = 128;
    config["height"] = 64;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject busDependency = deps.createNestedObject();
    busDependency["role"] = "i2c_bus";
    busDependency["deviceId"] = 12;
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
    TEST_ASSERT_EQUAL_STRING("#FF0000", root["backgroundColor"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("#00FF00", root["pages"][0]["widgets"][0]["color"].as<const char*>());
    TEST_ASSERT_TRUE(root["pages"][0]["widgets"][2]["color"].isNull());

    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(parseDisplayLayoutJson(root, decoded));
    TEST_ASSERT_EQUAL_HEX16(original.backgroundColor, decoded.backgroundColor);
    TEST_ASSERT_EQUAL_UINT8(original.schemaVersion, decoded.schemaVersion);
    TEST_ASSERT_EQUAL_UINT8(original.pages.size(), decoded.pages.size());
    TEST_ASSERT_EQUAL_STRING(original.pages[0].id, decoded.pages[0].id);
    TEST_ASSERT_EQUAL_UINT8(original.pages[0].widgets[1].bindingKind, decoded.pages[0].widgets[1].bindingKind);
    TEST_ASSERT_EQUAL_UINT16(original.pages[0].widgets[0].refreshIntervalMs, decoded.pages[0].widgets[0].refreshIntervalMs);
    TEST_ASSERT_EQUAL_HEX16(original.pages[0].widgets[0].color, decoded.pages[0].widgets[0].color);
    TEST_ASSERT_EQUAL_STRING(original.pages[0].widgets[1].text, decoded.pages[0].widgets[1].text);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutWidgetType::Bitmap), decoded.pages[0].widgets[2].type);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutBitmapFormat::Mono1), decoded.pages[0].widgets[2].bitmapFormat);
    TEST_ASSERT_EQUAL_UINT8(1, decoded.pages[0].widgets[2].keepAspectRatio);
    TEST_ASSERT_EQUAL_UINT8(4, decoded.pages[0].widgets[2].bitmapData.size());
    TEST_ASSERT_EQUAL_UINT8(0, decoded.activePageIndex);
}

void test_ssd1306_layout_codec_migrates_v4_colors() {
    DisplayLayoutBinaryHeaderV1 header{};
    header.recordVersion = 4U;
    header.deviceId = 77U;
    header.schemaVersion = kDisplayLayoutSchemaVersion;
    header.pageCount = 1U;

    DisplayLayoutBinaryPageHeaderV1 pageHeader{};
    pageHeader.widgetCount = 1U;
    std::snprintf(pageHeader.id, sizeof(pageHeader.id), "%s", "main");
    std::snprintf(pageHeader.name, sizeof(pageHeader.name), "%s", "Main");

    DisplayLayoutBinaryWidgetV4 widget{};
    std::snprintf(widget.id, sizeof(widget.id), "%s", "text");
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Text);
    widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    widget.width = 20U;
    widget.height = 8U;
    std::snprintf(widget.text, sizeof(widget.text), "%s", "legacy");

    std::vector<uint8_t> blob;
    const auto append = [&blob](const auto& value) {
        const size_t offset = blob.size();
        blob.resize(offset + sizeof(value));
        std::memcpy(blob.data() + offset, &value, sizeof(value));
    };
    append(header);
    append(pageHeader);
    append(widget);

    DisplayLayoutRecordV1 decoded{};
    TEST_ASSERT_TRUE(decodeDisplayLayoutBinary(blob.data(), blob.size(), decoded));
    TEST_ASSERT_EQUAL_UINT16(kDisplayLayoutRecordVersion, decoded.recordVersion);
    TEST_ASSERT_EQUAL_HEX16(0U, decoded.backgroundColor);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFU, decoded.pages[0].widgets[0].color);
    TEST_ASSERT_EQUAL_STRING("legacy", decoded.pages[0].widgets[0].text);
}

void test_ssd1306_layout_codec_emits_single_page_when_filtered() {
    DisplayLayoutRecordV1 layout = makeLayoutRecord();
    DisplayLayoutPageV1 second{};
    std::snprintf(second.id, sizeof(second.id), "%s", "page2");
    std::snprintf(second.name, sizeof(second.name), "%s", "Second");
    DisplayLayoutWidgetV1 widget{};
    widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    std::snprintf(widget.text, sizeof(widget.text), "%s", "p2");
    second.widgets.push_back(widget);
    layout.pages.push_back(second);

    DynamicJsonDocument doc(4096);
    JsonObject root = doc.to<JsonObject>();
    writeDisplayLayoutJson(layout, root, 1); // ?page=1 -> only the second page

    JsonArrayConst pages = root["pages"].as<JsonArrayConst>();
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(pages.size()));
    TEST_ASSERT_EQUAL_STRING("page2", pages[0]["id"].as<const char*>());
    // The active page id is unchanged by the filter (still the record's active page).
    TEST_ASSERT_EQUAL_STRING("main", root["activePageId"].as<const char*>());
}

void test_ssd1306_config_migrates_v1_v2_v3_without_bus_id() {
    Ssd1306DeviceConfigV1 v1{};
    TEST_ASSERT_TRUE(assignDeviceBaseConfig(v1, "OLED V1", true));
    v1.i2cBusDeviceId = 11;
    v1.i2cAddress = 0x3D;
    v1.layoutWidth = 96;
    v1.layoutHeight = 16;

    Ssd1306DeviceConfigV2 v2{};
    TEST_ASSERT_TRUE(assignDeviceBaseConfig(v2, "OLED V2", true));
    v2.i2cBusDeviceId = 22;
    v2.i2cAddress = 0x3E;
    v2.width = 128;
    v2.height = 32;

    Ssd1306DeviceConfigV3 v3{};
    TEST_ASSERT_TRUE(assignDeviceBaseConfig(v3, "OLED V3", true));
    v3.i2cBusDeviceId = 33;
    v3.i2cAddress = 0x3F;
    v3.rotation = 2;
    v3.width = 128;
    v3.height = 64;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    Ssd1306DeviceConfigV4 migrated{};

    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ssd1306DeviceConfigV1::kMagic, v1, buffer, ssd1306DeviceConfigV1Size()));
    TEST_ASSERT_TRUE(decodeSsd1306DeviceConfig(buffer, ssd1306DeviceConfigV1Size(), migrated));
    TEST_ASSERT_EQUAL_STRING("OLED V1", migrated.name);
    TEST_ASSERT_EQUAL_UINT8(0x3D, migrated.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(0, migrated.rotation);
    TEST_ASSERT_EQUAL_UINT16(96, migrated.width);
    TEST_ASSERT_EQUAL_UINT16(16, migrated.height);

    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ssd1306DeviceConfigV2::kMagic, v2, buffer, ssd1306DeviceConfigSize(v2)));
    TEST_ASSERT_TRUE(decodeSsd1306DeviceConfig(buffer, ssd1306DeviceConfigSize(v2), migrated));
    TEST_ASSERT_EQUAL_STRING("OLED V2", migrated.name);
    TEST_ASSERT_EQUAL_UINT8(0x3E, migrated.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(0, migrated.rotation);
    TEST_ASSERT_EQUAL_UINT16(128, migrated.width);
    TEST_ASSERT_EQUAL_UINT16(32, migrated.height);

    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ssd1306DeviceConfigV3::kMagic, v3, buffer, ssd1306DeviceConfigSize(v3)));
    TEST_ASSERT_TRUE(decodeSsd1306DeviceConfig(buffer, ssd1306DeviceConfigSize(v3), migrated));
    TEST_ASSERT_EQUAL_STRING("OLED V3", migrated.name);
    TEST_ASSERT_EQUAL_UINT8(0x3F, migrated.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(2, migrated.rotation);
    TEST_ASSERT_EQUAL_UINT16(128, migrated.width);
    TEST_ASSERT_EQUAL_UINT16(64, migrated.height);
}

void test_ssd1306_registry_migrates_v3_blob_and_preserves_bus_dependency() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DeviceRegistryEntry busRecord{};
    busRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    busRecord.header.deviceId = 7301U;
    busRecord.header.typeId = I2cBusDevice::descriptor().typeId;
    busRecord.header.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    busRecord.header.configRevision = 1U;
    busRecord.status = DeviceStatus::Ready;

    DeviceRegistryEntry displayRecord{};
    displayRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    displayRecord.header.deviceId = 7302U;
    displayRecord.header.typeId = Ssd1306Device::descriptor().typeId;
    displayRecord.header.configVersion = 3U;
    displayRecord.header.configRevision = 1U;
    displayRecord.depCount = 1U;
    displayRecord.deps[0] = {DeviceRole::I2CBus, busRecord.header.deviceId};
    displayRecord.status = DeviceStatus::Ready;

    I2cBusDeviceConfigV1 busConfig{};
    TEST_ASSERT_TRUE(assignDeviceBaseConfig(busConfig, "i2c-bus", true));
    busConfig.sdaPin = 21U;
    busConfig.sclPin = 22U;
    uint8_t busBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, busConfig, busBuffer, i2cBusDeviceConfigSize(busConfig)));
    DeviceConfigBlob busBlob{};
    TEST_ASSERT_TRUE(busBlob.assign(busBuffer, i2cBusDeviceConfigSize(busConfig)));

    Ssd1306DeviceConfigV3 legacy{};
    TEST_ASSERT_TRUE(assignDeviceBaseConfig(legacy, "legacy-oled", true));
    legacy.i2cBusDeviceId = 9999U;
    legacy.i2cAddress = 0x3DU;
    legacy.rotation = 1U;
    legacy.width = 128U;
    legacy.height = 32U;
    uint8_t legacyBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ssd1306DeviceConfigV3::kMagic, legacy, legacyBuffer, ssd1306DeviceConfigSize(legacy)));
    DeviceConfigBlob legacyBlob{};
    TEST_ASSERT_TRUE(legacyBlob.assign(legacyBuffer, ssd1306DeviceConfigSize(legacy)));

    DeviceRegistrySnapshot snapshot{};
    snapshot.records = {busRecord, displayRecord};
    snapshot.indexEntries = {{busRecord.header.deviceId, busRecord.header.typeId},
                             {displayRecord.header.deviceId, displayRecord.header.typeId}};
    DeviceConfigBlobMap configBlobs{};
    configBlobs[busRecord.header.deviceId] = busBlob;
    configBlobs[displayRecord.header.deviceId] = legacyBlob;
    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());

    SequentialDeviceIdSource ids(7400U);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0U).ok());

    const Ssd1306Device* runtime = static_cast<const Ssd1306Device*>(registry.runtime(displayRecord.header.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_UINT32(Ssd1306Device::descriptor().currentConfigVersion, runtime->configVersion());
    TEST_ASSERT_EQUAL_UINT32(busRecord.header.deviceId, runtime->dependencyDeviceId(DeviceRole::I2CBus));
    TEST_ASSERT_EQUAL_UINT8(0x3D, runtime->config().i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(1, runtime->config().rotation);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
}

void test_ssd1306_api_adapter_streams_layout_and_setup_extension() {
    Ssd1306DeviceConfigV4 config{};
    TEST_ASSERT_TRUE(assignDeviceBaseConfig(config, "OLED", true));
    config.i2cAddress = 0x3C;
    config.width = 128;
    config.height = 64;

    uint8_t configBuffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob configBlob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ssd1306DeviceConfigV4::kMagic, config, configBuffer, ssd1306DeviceConfigSize(config)));
    TEST_ASSERT_TRUE(configBlob.assign(configBuffer, ssd1306DeviceConfigSize(config)));
    DeviceRegistryEntry record{};
    record.header.deviceId = 42;
    record.header.typeId = Ssd1306Device::descriptor().typeId;
    record.header.configVersion = Ssd1306Device::descriptor().currentConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    Ssd1306Device device(record, configBlob);

    DisplayLayoutRecordV1 layout = makeLayoutRecord();
    DisplayLayoutPageV1 secondPage{};
    std::snprintf(secondPage.id, sizeof(secondPage.id), "%s", "second");
    std::snprintf(secondPage.name, sizeof(secondPage.name), "%s", "Second");
    layout.pages.push_back(secondPage);
    std::vector<uint8_t> layoutBlob;
    TEST_ASSERT_TRUE(encodeDisplayLayoutBinary(layout, layoutBlob));
    TEST_ASSERT_TRUE(device.applyPersistedStateUpdate(layoutBlob.data(), layoutBlob.size()).ok());

    std::unique_ptr<IJsonChunkProducer> layoutProducer = Ssd1306DeviceApiAdapter::instance().createLayoutJsonProducer(device, 1);
    TEST_ASSERT_NOT_NULL(layoutProducer.get());
    StringJsonChunkSink layoutSink;
    while (layoutProducer->next(layoutSink)) {
    }
    DynamicJsonDocument layoutDocument(4096);
    TEST_ASSERT_FALSE(deserializeJson(layoutDocument, layoutSink.output));
    TEST_ASSERT_TRUE(layoutDocument["success"].as<bool>());
    TEST_ASSERT_EQUAL_UINT32(1U, layoutDocument["pages"].size());
    TEST_ASSERT_EQUAL_STRING("second", layoutDocument["pages"][0]["id"].as<const char*>());

    std::unique_ptr<IJsonChunkProducer> setupProducer = Ssd1306DeviceApiAdapter::instance().createSetupExportJsonProducer(device);
    TEST_ASSERT_NOT_NULL(setupProducer.get());
    StringJsonChunkSink setupSink;
    while (setupProducer->next(setupSink)) {
    }
    size_t offset = 0U;
    size_t beginCount = 0U;
    size_t pageCount = 0U;
    size_t widgetCount = 0U;
    size_t endCount = 0U;
    while (offset < setupSink.output.size()) {
        const size_t end = setupSink.output.find('\n', offset);
        const std::string line = setupSink.output.substr(offset, end - offset);
        DynamicJsonDocument setupRecord(8192);
        TEST_ASSERT_FALSE(deserializeJson(setupRecord, line));
        const char* kind = setupRecord["kind"] | "";
        if (std::strcmp(kind, "layout_begin") == 0) {
            ++beginCount;
            TEST_ASSERT_EQUAL_UINT32(2U, setupRecord["pageCount"].as<uint32_t>());
        } else if (std::strcmp(kind, "layout_page") == 0) {
            ++pageCount;
        } else if (std::strcmp(kind, "layout_widget") == 0) {
            ++widgetCount;
        } else if (std::strcmp(kind, "layout_end") == 0) {
            ++endCount;
        } else {
            TEST_FAIL_MESSAGE("unexpected setup export record kind");
        }
        offset = end == std::string::npos ? setupSink.output.size() : end + 1U;
    }
    TEST_ASSERT_EQUAL_UINT32(1U, beginCount);
    TEST_ASSERT_EQUAL_UINT32(2U, pageCount);
    TEST_ASSERT_EQUAL_UINT32(3U, widgetCount);
    TEST_ASSERT_EQUAL_UINT32(1U, endCount);
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
    TEST_ASSERT_EQUAL_HEX16(original.backgroundColor, loaded.backgroundColor);
    TEST_ASSERT_EQUAL_HEX16(original.pages[0].widgets[0].color, loaded.pages[0].widgets[0].color);
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
    createDoc["config"]["deps"][0]["deviceId"] = busResult.deviceId;
    DeviceCreateRequest createRequest{};
    TEST_ASSERT_TRUE(Ssd1306DeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), createRequest, error));
    TEST_ASSERT_NULL(error);
    const DeviceCreateResult createResult = registry.create(createRequest, 0);
    TEST_ASSERT_TRUE(createResult.ok());

    IDeviceRuntime* runtime = registry.runtime(createResult.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);

    StaticJsonDocument<2048> updateDoc;
    fillSsd1306DeviceDocument(updateDoc, true);
    DeviceConfigUpdateRequest updateRequest{};
    TEST_ASSERT_TRUE(
        Ssd1306DeviceApiAdapter::instance().parseUpdateConfigRequest(updateDoc.as<JsonObjectConst>(), *runtime, updateRequest, error));
    TEST_ASSERT_NULL(error);
    TEST_ASSERT_TRUE(updateRequest.persistedStateProvided);

    const DeviceMutationResult updateResult =
        registry.updateConfigAndDeps(createResult.deviceId, updateRequest.configBlob, updateRequest.configVersion, updateRequest.baseConfig,
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

void test_ssd1306_api_adapter_partial_update_preserves_bus_and_dimensions() {
    Ssd1306DeviceConfigV4 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "ssd1306");
    // i2cAddress/width/height are deliberately non-default (compiled defaults are 0x3C/128/64) so
    // these assertions cannot pass by accident if the merge fix regresses and the fields are
    // silently reset to their struct defaults instead of the runtime's current values.
    config.i2cAddress = 0x50;
    config.width = 64;
    config.height = 32;
    config.rotation = 0;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ssd1306DeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ssd1306DeviceConfigV4::kMagic, config, buffer, size));
    DeviceConfigBlob configBlob{};
    TEST_ASSERT_TRUE(configBlob.assign(buffer, size));

    DeviceRegistryEntry record{};
    record.header.deviceId = 200;
    record.header.typeId = Ssd1306Device::descriptor().typeId;
    record.header.configVersion = Ssd1306Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::I2CBus, 12};
    record.status = DeviceStatus::Ready;

    Ssd1306Device device(record, configBlob);

    StaticJsonDocument<128> doc;
    JsonObject updateConfig = doc.createNestedObject("config");
    updateConfig["rotation"] = 2;

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Ssd1306DeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), device, request, error), error);
    TEST_ASSERT_FALSE_MESSAGE(request.depsProvided, "depsProvided must stay false when the update carries no multi-source layout");
    TEST_ASSERT_EQUAL_UINT8(1U, request.depCount);
    TEST_ASSERT_EQUAL_UINT32(12U, request.deps[0].deviceId);

    Ssd1306DeviceConfigV4 parsed{};
    TEST_ASSERT_TRUE(
        decodeSsd1306DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(2, parsed.rotation);
    TEST_ASSERT_EQUAL_UINT8(0x50, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT16(64, parsed.width);
    TEST_ASSERT_EQUAL_UINT16(32, parsed.height);
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

void test_ssd1306_config_rejects_removed_i2c_bus_device_id() {
    StaticJsonDocument<1024> doc;
    fillSsd1306DeviceDocument(doc, false);
    JsonObject config = doc["config"].as<JsonObject>();
    config["i2cBusDeviceId"] = 4249059392UL;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(Ssd1306DeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("ssd1306 i2cBusDeviceId must be provided through deps", error);
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
    firstConfig["deps"][0]["deviceId"] = busResult.deviceId;
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
    secondConfig["deps"][0]["deviceId"] = busResult.deviceId;
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
