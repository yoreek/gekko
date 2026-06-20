#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "portal/DashboardLayoutStore.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {
struct FixedDeviceIdSource final : public IDeviceIdSource {
    explicit FixedDeviceIdSource(std::initializer_list<DeviceId> ids) : ids_(ids) {}

    bool next(DeviceId& out) override {
        if (index_ >= ids_.size()) {
            return false;
        }
        out = ids_[index_++];
        return true;
    }

    std::vector<DeviceId> ids_{};
    size_t index_{0};
};

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyConfig(const DummyDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(ewfm::encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

DeviceCreateRequest makeDummyCreateRequest(const std::string& name) {
    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", name.c_str());

    DeviceCreateRequest request{};
    request.typeId = DummyDevice::descriptor().typeId;
    request.name = name;
    request.enabled = true;
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeDummyConfig(config);
    request.persistencePolicy = DevicePersistencePolicy::Immediate;
    return request;
}

DashboardLayoutSnapshot makeLayout() {
    DashboardLayoutSnapshot layout{};
    layout.schemaVersion = DashboardLayoutStore::kSchemaVersion;
    layout.activePanelId = "main";

    DashboardPanelLayout panel{};
    panel.id = "main";
    panel.name = "Main";
    panel.order = 0;
    panel.widgets.push_back({101, 0, 0, 1, 1});

    layout.panels.push_back(panel);
    return layout;
}

void addPanel(DashboardLayoutSnapshot& layout, const char* id, const char* name) {
    DashboardPanelLayout panel{};
    panel.id = id;
    panel.name = name;
    panel.order = static_cast<uint8_t>(layout.panels.size());
    layout.panels.push_back(panel);
}
} // namespace

void test_dashboard_layout_default_generation_and_revisioned_save_load() {
    MemoryConfigStorage storage;
    DashboardLayoutStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    DashboardLayoutLoadResult empty = store.load();
    TEST_ASSERT_TRUE(empty.defaulted);
    TEST_ASSERT_EQUAL_UINT32(0, empty.revision);
    TEST_ASSERT_EQUAL_UINT32(1, empty.layout.panels.size());
    TEST_ASSERT_EQUAL_STRING("main", empty.layout.activePanelId.c_str());

    DashboardLayoutSnapshot layout = makeLayout();
    DashboardLayoutSaveResult firstSave = store.save(layout);
    TEST_ASSERT_TRUE(firstSave.ok());
    TEST_ASSERT_EQUAL_UINT32(1, firstSave.revision);

    std::vector<uint8_t> storedBlob;
    TEST_ASSERT_TRUE(storage.getBlob("layout_blob", storedBlob));
    TEST_ASSERT_TRUE(storedBlob.size() > 4);
    TEST_ASSERT_EQUAL_CHAR('D', static_cast<char>(storedBlob[0]));
    TEST_ASSERT_EQUAL_CHAR('L', static_cast<char>(storedBlob[1]));
    TEST_ASSERT_EQUAL_CHAR('B', static_cast<char>(storedBlob[2]));
    TEST_ASSERT_EQUAL_CHAR('1', static_cast<char>(storedBlob[3]));

    DashboardLayoutSaveResult secondSave = store.save(firstSave.layout);
    TEST_ASSERT_TRUE(secondSave.ok());
    TEST_ASSERT_EQUAL_UINT32(2, secondSave.revision);

    DashboardLayoutLoadResult loaded = store.load();
    TEST_ASSERT_FALSE(loaded.defaulted);
    TEST_ASSERT_EQUAL_UINT32(2, loaded.revision);
    TEST_ASSERT_EQUAL_UINT32(1, loaded.layout.panels.size());
    TEST_ASSERT_EQUAL_UINT32(101, loaded.layout.panels[0].widgets[0].deviceId);
}

void test_dashboard_layout_validation_limits_and_duplicates() {
    MemoryConfigStorage storage;
    DashboardLayoutStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    DashboardLayoutSnapshot layout = makeLayout();
    TEST_ASSERT_TRUE(store.validate(layout).ok());

    DashboardLayoutSnapshot tooMany = layout;
    for (uint8_t index = 2; index <= 9; ++index) {
        std::string id = "panel-" + std::to_string(index);
        std::string name = "Panel " + std::to_string(index);
        addPanel(tooMany, id.c_str(), name.c_str());
    }
    TEST_ASSERT_EQUAL(DashboardLayoutError::TooManyPanels, store.validate(tooMany).error);

    DashboardLayoutSnapshot longName = layout;
    longName.panels[0].name = "123456789012345678901234567890123";
    TEST_ASSERT_EQUAL(DashboardLayoutError::PanelNameTooLong, store.validate(longName).error);

    DashboardLayoutSnapshot duplicateName = layout;
    addPanel(duplicateName, "second", "main");
    TEST_ASSERT_EQUAL(DashboardLayoutError::DuplicatePanelName, store.validate(duplicateName).error);

    DashboardLayoutSnapshot badActive = layout;
    badActive.activePanelId = "missing";
    TEST_ASSERT_EQUAL(DashboardLayoutError::InvalidActivePanel, store.validate(badActive).error);

    DashboardLayoutSnapshot duplicateWidget = layout;
    duplicateWidget.panels[0].widgets.push_back({101, 1, 0, 1, 1});
    TEST_ASSERT_EQUAL(DashboardLayoutError::DuplicateWidget, store.validate(duplicateWidget).error);

    DashboardLayoutSnapshot invalidWidget = layout;
    invalidWidget.panels[0].widgets[0].w = 0;
    TEST_ASSERT_EQUAL(DashboardLayoutError::InvalidWidget, store.validate(invalidWidget).error);
}

void test_dashboard_layout_save_json_rejects_invalid_shape_and_schema() {
    MemoryConfigStorage storage;
    DashboardLayoutStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    DynamicJsonDocument doc(1024);
    doc["schema_version"] = 99;
    doc["active_panel_id"] = "main";
    JsonArray panels = doc.createNestedArray("panels");
    JsonObject panel = panels.createNestedObject();
    panel["id"] = "main";
    panel["name"] = "Main";
    panel["order"] = 0;
    panel.createNestedArray("widgets");

    DashboardLayoutSaveResult schemaResult = store.saveJson(doc.as<JsonVariantConst>());
    TEST_ASSERT_FALSE(schemaResult.ok());
    TEST_ASSERT_EQUAL(DashboardLayoutError::UnsupportedSchema, schemaResult.validation.error);

    doc.clear();
    doc["schema_version"] = DashboardLayoutStore::kSchemaVersion;
    doc["active_panel_id"] = "main";
    panels = doc.createNestedArray("panels");
    panel = panels.createNestedObject();
    panel["id"] = "main";
    panel["name"] = "Main";
    JsonArray widgets = panel.createNestedArray("widgets");
    JsonObject widget = widgets.createNestedObject();
    widget["device_id"] = 101;
    widget["x"] = -1;
    widget["y"] = 0;
    widget["w"] = 1;
    widget["h"] = 1;

    DashboardLayoutSaveResult widgetResult = store.saveJson(doc.as<JsonVariantConst>());
    TEST_ASSERT_FALSE(widgetResult.ok());
    TEST_ASSERT_EQUAL(DashboardLayoutError::InvalidWidget, widgetResult.validation.error);

    doc.clear();
    doc["schema_version"] = DashboardLayoutStore::kSchemaVersion;
    doc["active_panel_id"] = "main";
    panels = doc.createNestedArray("panels");
    panel = panels.createNestedObject();
    panel["id"] = "main";
    panel["name"] = "Main";
    widgets = panel.createNestedArray("widgets");
    JsonArray compactWidget = widgets.createNestedArray();
    compactWidget.add(101);
    compactWidget.add(0);
    compactWidget.add(0);
    compactWidget.add(1);
    compactWidget.add(1);

    DashboardLayoutSaveResult compactResult = store.saveJson(doc.as<JsonVariantConst>());
    TEST_ASSERT_TRUE(compactResult.ok());
    TEST_ASSERT_EQUAL_UINT32(101, compactResult.layout.panels[0].widgets[0].deviceId);

    DynamicJsonDocument roundTripDoc(512);
    JsonObject roundTripRoot = roundTripDoc.to<JsonObject>();
    store.writeLayoutJson(roundTripRoot, compactResult.layout);
    TEST_ASSERT_TRUE(roundTripRoot["panels"][0]["widgets"][0].is<JsonArrayConst>());
    TEST_ASSERT_EQUAL_UINT32(101, roundTripRoot["panels"][0]["widgets"][0][0].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(1, roundTripRoot["panels"][0]["widgets"][0][3].as<uint32_t>());
}

void test_dashboard_layout_prunes_stale_registry_devices() {
    MemoryConfigStorage registryStorage;
    DeviceRegistryStore registryStore(registryStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));

    FixedDeviceIdSource idSource({101, 102});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("dummy-a"), 10).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("dummy-b"), 20).ok());

    MemoryConfigStorage layoutStorage;
    DashboardLayoutStore layoutStore(layoutStorage, &registry);
    TEST_ASSERT_TRUE(layoutStore.begin());

    DashboardLayoutSnapshot layout = layoutStore.defaultLayout();
    DashboardLayoutSaveResult save = layoutStore.save(layout);
    TEST_ASSERT_TRUE(save.ok());
    TEST_ASSERT_EQUAL_UINT32(2, save.layout.panels[0].widgets.size());

    TEST_ASSERT_TRUE(registry.remove(102, 30).ok());

    DashboardLayoutLoadResult loaded = layoutStore.load();
    TEST_ASSERT_FALSE(loaded.defaulted);
    TEST_ASSERT_EQUAL_UINT32(1, loaded.layout.panels[0].widgets.size());
    TEST_ASSERT_EQUAL_UINT32(101, loaded.layout.panels[0].widgets[0].deviceId);
}

void test_dashboard_layout_full_prune_recovers_to_default() {
    MemoryConfigStorage registryStorage;
    DeviceRegistryStore registryStore(registryStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));

    FixedDeviceIdSource idSource({101, 102});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("dummy-a"), 10).ok());

    MemoryConfigStorage layoutStorage;
    DashboardLayoutStore seedStore(layoutStorage);
    TEST_ASSERT_TRUE(seedStore.begin());

    DashboardLayoutSnapshot stale = makeLayout();
    stale.panels[0].widgets[0].deviceId = 999;
    DashboardLayoutSaveResult save = seedStore.save(stale);
    TEST_ASSERT_TRUE(save.ok());
    TEST_ASSERT_EQUAL_UINT32(1, save.layout.panels[0].widgets.size());

    DashboardLayoutStore layoutStore(layoutStorage, &registry);
    TEST_ASSERT_TRUE(layoutStore.begin());

    DashboardLayoutLoadResult loaded = layoutStore.load();
    TEST_ASSERT_TRUE(loaded.defaulted);
    TEST_ASSERT_EQUAL_UINT32(1, loaded.layout.panels[0].widgets.size());
    TEST_ASSERT_EQUAL_UINT32(101, loaded.layout.panels[0].widgets[0].deviceId);
}

void test_dashboard_layout_legacy_json_storage_resets_to_default() {
    MemoryConfigStorage storage;
    TEST_ASSERT_TRUE(storage.begin("dashboard", false));

    DynamicJsonDocument doc(512);
    doc["schema_version"] = DashboardLayoutStore::kSchemaVersion;
    doc["active_panel_id"] = "main";
    JsonArray panels = doc.createNestedArray("panels");
    JsonObject panel = panels.createNestedObject();
    panel["id"] = "main";
    panel["name"] = "Main";
    panel["order"] = 0;
    JsonArray widgets = panel.createNestedArray("widgets");
    JsonObject widget = widgets.createNestedObject();
    widget["device_id"] = 101;
    widget["x"] = 0;
    widget["y"] = 0;
    widget["w"] = 1;
    widget["h"] = 1;

    std::string legacyJson;
    serializeJson(doc, legacyJson);
    TEST_ASSERT_TRUE(storage.putString("layout", legacyJson));
    TEST_ASSERT_TRUE(storage.putUInt("revision", 7));

    DashboardLayoutStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    DashboardLayoutLoadResult loaded = store.load();
    TEST_ASSERT_TRUE(loaded.defaulted);
    TEST_ASSERT_EQUAL_UINT32(7, loaded.revision);
    TEST_ASSERT_EQUAL_UINT32(1, loaded.layout.panels.size());
    TEST_ASSERT_EQUAL_UINT32(0, loaded.layout.panels[0].widgets.size());
    TEST_ASSERT_FALSE(storage.hasKey("layout"));
    TEST_ASSERT_FALSE(storage.hasKey("layout_blob"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_dashboard_layout_default_generation_and_revisioned_save_load);
    RUN_TEST(test_dashboard_layout_validation_limits_and_duplicates);
    RUN_TEST(test_dashboard_layout_save_json_rejects_invalid_shape_and_schema);
    RUN_TEST(test_dashboard_layout_prunes_stale_registry_devices);
    RUN_TEST(test_dashboard_layout_full_prune_recovers_to_default);
    RUN_TEST(test_dashboard_layout_legacy_json_storage_resets_to_default);
    return UNITY_END();
}
