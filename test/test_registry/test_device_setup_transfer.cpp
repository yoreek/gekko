#include "../test_devices/JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayLayoutStore.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceSetupTransferCodec.h"
#include "integrations/common/DeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <unity.h>

using namespace ewfm;

namespace {
void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}

struct FixedDeviceIdSource final : public IDeviceIdSource {
    explicit FixedDeviceIdSource(std::initializer_list<DeviceId> ids) : ids_(ids) {}

    bool next(DeviceId& out) override {
        if (index_ >= ids_.size()) {
            return false;
        }
        out = ids_[index_++];
        return true;
    }

    std::vector<DeviceId> ids_;
    size_t index_{0};
};

struct FailingStorage final : public IConfigStorage {
    bool begin(const char* namespaceName, bool readOnly) override {
        return storage_.begin(namespaceName, readOnly);
    }
    void end() override {
        storage_.end();
    }
    bool hasKey(const char* key) const override {
        return storage_.hasKey(key);
    }
    bool putString(const char* key, const std::string& value) override {
        return storage_.putString(key, value);
    }
    bool getString(const char* key, std::string& value) const override {
        return storage_.getString(key, value);
    }
    bool putBlob(const char* key, const uint8_t* value, size_t size) override {
        if (failNextBlob_) {
            failNextBlob_ = false;
            return false;
        }
        return storage_.putBlob(key, value, size);
    }
    bool getBlob(const char* key, uint8_t* value, size_t& size) const override {
        return storage_.getBlob(key, value, size);
    }
    bool putBlob(const char* key, const std::vector<uint8_t>& value) override {
        return putBlob(key, value.data(), value.size());
    }
    bool getBlob(const char* key, std::vector<uint8_t>& value) const override {
        return storage_.getBlob(key, value);
    }
    bool putUInt(const char* key, uint32_t value) override {
        return storage_.putUInt(key, value);
    }
    bool getUInt(const char* key, uint32_t& value) const override {
        return storage_.getUInt(key, value);
    }
    bool putBool(const char* key, bool value) override {
        return storage_.putBool(key, value);
    }
    bool getBool(const char* key, bool& value) const override {
        return storage_.getBool(key, value);
    }
    bool remove(const char* key) override {
        return storage_.remove(key);
    }
    bool clear() override {
        return storage_.clear();
    }

    void failNextBlobWrite() {
        failNextBlob_ = true;
    }

    MemoryConfigStorage storage_;
    bool failNextBlob_{false};
};

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyConfig(const DummyDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(DeviceBaseConfigV1::kMagic, config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

void writeTextFile(const char* path, const std::string& text) {
    std::ofstream file(path, std::ios::trunc);
    TEST_ASSERT_TRUE(file.is_open());
    file << text;
    file.close();
}

DeviceCreateRequest makeDummyCreateRequest(const char* name) {
    DummyDeviceConfigV1 config{};
    config.enabled = 1U;
    TEST_ASSERT_TRUE(copyBoundedText(config.name, name));
    DeviceCreateRequest request{};
    request.typeId = DummyDevice::descriptor().typeId;
    TEST_ASSERT_TRUE(request.assignName(name));
    request.configBlob = encodeDummyConfig(config);
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.setEnabled(true);
    return request;
}

const DeviceApiAdapterRegistry& transferAdapters() {
    static const DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    return adapters;
}

// Creates a device through its REST adapter from a create-request JSON, mirroring the
// controller flow so per-type binary encoding stays out of the test.
DeviceId createFromJson(DeviceRegistry& registry, const char* typeName, const char* json) {
    DynamicJsonDocument doc(8192);
    TEST_ASSERT_TRUE(deserializeJson(doc, json) == DeserializationError::Ok);
    const IDeviceApiAdapter* adapter = transferAdapters().findByName(typeName);
    TEST_ASSERT_NOT_NULL_MESSAGE(adapter, typeName);

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(adapter->parseCreateRequest(doc.as<JsonObjectConst>(), request, error),
                             error != nullptr ? error : "parseCreateRequest failed");
    auto persisted = std::make_unique<DeviceCreatePersistenceRequest>();
    error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(adapter->parseCreatePersistedStateRequest(doc.as<JsonObjectConst>(), request, *persisted, error),
                             error != nullptr ? error : "parseCreatePersistedStateRequest failed");

    const DeviceCreateResult result = registry.create(request, 0);
    TEST_ASSERT_TRUE_MESSAGE(result.ok(), result.validation.message);
    if (persisted->persistedStateProvided) {
        const DeviceValidationResult stateResult = registry.applyPersistedStateUpdate(result.deviceId, persisted->persistedStateBlob.data(),
                                                                                      persisted->persistedStateBlob.size(), 0);
        TEST_ASSERT_TRUE_MESSAGE(stateResult.ok(), stateResult.message);
    }
    return result.deviceId;
}
} // namespace

void test_device_setup_export_includes_metadata_and_redacts_secret_strings() {
    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{1001};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("Aquarium Lamp"), 0).ok());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, transferAdapters(), registry.registryRevision()));
    size_t firstLineEnd = bundle.find('\n');
    TEST_ASSERT_TRUE(firstLineEnd != std::string::npos);
    DynamicJsonDocument envelopeDoc(512);
    TEST_ASSERT_FALSE(deserializeJson(envelopeDoc, bundle.substr(0, firstLineEnd)));
    assertMatchesJsonSchema("schemas/rest/v1/bundle/transfer-envelope.schema.json", envelopeDoc.as<JsonVariantConst>());
    TEST_ASSERT_TRUE(bundle.find("transferSchemaVersion") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"record\":{\"id\":") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"typeName\":\"dummy\"") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"configVersion\":") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"deps\":[]") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("config_blob_hex") == std::string::npos);
    TEST_ASSERT_EQUAL(std::string::npos, bundle.find("\"status\""));
    TEST_ASSERT_EQUAL(std::string::npos, bundle.find("persistencePolicy"));
    TEST_ASSERT_EQUAL(std::string::npos, bundle.find("password"));
}

void test_device_setup_export_round_trips_back_into_registry() {
    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{1001};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("Aquarium Lamp"), 0).ok());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, transferAdapters(), registry.registryRevision()));

    const char* path = "/tmp/device_setup_transfer_bundle.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE(parsed.ok());
    TEST_ASSERT_EQUAL_UINT32(1U, parsed.deviceCount);

    MemoryConfigStorage restoreStorage;
    FixedDeviceIdSource restoreIdSource{2001};
    DeviceRegistryStore restoreRegistryStore(restoreStorage);
    DeviceRegistry restored(restoreRegistryStore, typeRegistry, restoreIdSource);
    TEST_ASSERT_TRUE(restored.restore(parsed.snapshot, parsed.configBlobs, parsed.registryRevision, 0).ok());
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(restored.list().size()));
    TEST_ASSERT_NOT_NULL(restored.runtime(1001));
    TEST_ASSERT_EQUAL_STRING("Aquarium Lamp", restored.runtime(1001)->name());
}

// Loud-failure guard: every runtime type registered in DeviceTypeRegistry must have a REST
// adapter, otherwise it silently falls out of setup transfer (the historical schedule /
// auto_switch bug). Fails as soon as a new type is added without one.
void test_device_setup_transfer_covers_all_registered_types() {
    const DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_FALSE(typeRegistry.descriptors().empty());
    for (const DeviceTypeDescriptor& descriptor : typeRegistry.descriptors()) {
        const IDeviceApiAdapter* adapter = transferAdapters().find(descriptor.typeId);
        TEST_ASSERT_NOT_NULL_MESSAGE(adapter, "device type has no REST adapter; setup transfer would skip it");
        TEST_ASSERT_NOT_NULL(adapter->typeName());
        const IDeviceApiAdapter* byName = transferAdapters().findByName(adapter->typeName());
        TEST_ASSERT_NOT_NULL(byName);
        TEST_ASSERT_EQUAL_UINT32(descriptor.typeId, byName->typeId());
        TEST_ASSERT_EQUAL_UINT32(descriptor.currentConfigVersion, adapter->currentConfigVersion());
    }
}

void test_device_setup_transfer_round_trips_previously_missing_types() {
    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{1001, 1002, 1003, 1004, 1005};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    const DeviceId switchId = createFromJson(registry, "gpio_switch", R"({"config":{"name":"Pump","enabled":true,"gpioPin":26}})");
    const DeviceId i2cBusId = createFromJson(registry, "i2c_bus",
                                             R"({"config":{"name":"Main I2C","enabled":true,"sdaPin":21,"sclPin":22,)"
                                             R"("frequencyHz":400000}})");
    const DeviceId scheduleId =
        createFromJson(registry, "schedule",
                       R"({"config":{"name":"Grow Light Plan","enabled":true,)"
                       R"("rules":[{"enabled":true,"weekDays":[1,2,3,4,5],"startMinuteOfDay":480,"endMinuteOfDay":1020}]}})");
    const std::string autoSwitchJson = std::string(R"({"config":{"name":"Auto Pump","enabled":true,"pauseDurationSeconds":300,)") +
                                       R"("deps":[{"role":"switch","deviceId":)" + std::to_string(switchId) +
                                       R"(},{"role":"condition","deviceId":)" + std::to_string(scheduleId) + R"(,"invert":true}]}})";
    const DeviceId autoSwitchId = createFromJson(registry, "auto_switch", autoSwitchJson.c_str());
    const std::string displayJson = std::string(R"({"config":{"name":"OLED","enabled":true,"deps":[{"role":"i2c_bus","deviceId":)") +
                                    std::to_string(i2cBusId) + R"(}],"i2cAddress":60,)" +
                                    R"("layout":{"pages":[{"id":"main","name":"Main","widgets":[]}]}}})";
    const DeviceId displayId = createFromJson(registry, "ssd1306", displayJson.c_str());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, transferAdapters(), registry.registryRevision()));
    TEST_ASSERT_TRUE(bundle.find("\"typeName\":\"schedule\"") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"typeName\":\"auto_switch\"") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"typeName\":\"ssd1306\"") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"transferSchemaVersion\":3") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"kind\":\"layout_begin\"") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"kind\":\"layout_page\"") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"kind\":\"layout_end\"") != std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"config\":{\"layout\"") == std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"role\":\"condition\",\"deviceId\":" + std::to_string(scheduleId) + ",\"invert\":true") !=
                     std::string::npos);
    TEST_ASSERT_TRUE(bundle.find("\"sdaPin\":21") != std::string::npos);
    TEST_ASSERT_EQUAL(std::string::npos, bundle.find("\"typeName\":\"unknown\""));

    const char* path = "/tmp/device_setup_transfer_full_types.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE_MESSAGE(parsed.ok(), parsed.errorMessage());
    TEST_ASSERT_EQUAL_UINT32(5U, parsed.deviceCount);

    // The display layout travels as editable JSON and is re-encoded into a persisted state blob.
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(parsed.persistedStateBlobs.size()));
    TEST_ASSERT_EQUAL_UINT32(displayId, parsed.persistedStateBlobs[0].deviceId);
    DisplayLayoutRecordV1 layout{};
    TEST_ASSERT_TRUE(
        decodeDisplayLayoutBinary(parsed.persistedStateBlobs[0].blob.data(), parsed.persistedStateBlobs[0].blob.size(), layout));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(layout.pages.size()));
    TEST_ASSERT_EQUAL_STRING("main", layout.pages[0].id);

    MemoryConfigStorage restoreStorage;
    FixedDeviceIdSource restoreIdSource{9001};
    DeviceRegistryStore restoreRegistryStore(restoreStorage);
    DeviceRegistry restored(restoreRegistryStore, typeRegistry, restoreIdSource);
    std::vector<DevicePersistedStateUpdate> persistedStateUpdates;
    for (const auto& entry : parsed.persistedStateBlobs) {
        persistedStateUpdates.push_back({entry.deviceId, entry.blob.data(), entry.blob.size()});
    }
    TEST_ASSERT_TRUE(restored.restore(parsed.snapshot, parsed.configBlobs, persistedStateUpdates, parsed.registryRevision, 0).ok());
    TEST_ASSERT_EQUAL_UINT32(5U, static_cast<uint32_t>(restored.list().size()));
    TEST_ASSERT_EQUAL_STRING("Grow Light Plan", restored.runtime(scheduleId)->name());
    TEST_ASSERT_EQUAL_STRING("Auto Pump", restored.runtime(autoSwitchId)->name());
    TEST_ASSERT_EQUAL_STRING("OLED", restored.runtime(displayId)->name());
    TEST_ASSERT_EQUAL_STRING("Main I2C", restored.runtime(i2cBusId)->name());
    TEST_ASSERT_EQUAL_UINT8(2U, restored.runtime(autoSwitchId)->dependencyCount());
    TEST_ASSERT_FALSE(restored.runtime(autoSwitchId)->dependencyLinks()[0].invert);
    TEST_ASSERT_TRUE(restored.runtime(autoSwitchId)->dependencyLinks()[1].invert);

    // Full fidelity check: re-exporting the restored registry yields the same bundle.
    std::string restoredBundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(restoredBundle, restored, transferAdapters(), parsed.registryRevision));
    TEST_ASSERT_EQUAL_STRING(bundle.c_str(), restoredBundle.c_str());
}

void test_device_setup_transfer_v3_parses_ordered_display_layout_records() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":3,\"deviceCount\":1}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":10,\"typeName\":\"ssd1306\"},"
                               "\"config\":{\"name\":\"OLED\",\"enabled\":true,\"deps\":[{\"role\":\"i2c_bus\","
                               "\"deviceId\":1}],\"i2cAddress\":60}}\n"
                               "{\"kind\":\"layout_begin\",\"deviceId\":10,\"schemaVersion\":1,\"activePageId\":\"main\",\"pageCount\":1}\n"
                               "{\"kind\":\"layout_page\",\"deviceId\":10,\"pageIndex\":0,\"id\":\"main\",\"name\":\"Main\","
                               "\"order\":0,\"widgetCount\":1}\n"
                               "{\"kind\":\"layout_widget\",\"deviceId\":10,\"pageIndex\":0,\"widgetIndex\":0,\"id\":\"title\","
                               "\"type\":\"text\",\"width\":20,\"height\":8,\"text\":\"Hello\"}\n"
                               "{\"kind\":\"layout_end\",\"deviceId\":10}\n";
    const char* path = "/tmp/device_setup_transfer_v3_display.ndjson";
    writeTextFile(path, bundle);

    const DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE_MESSAGE(parsed.ok(), parsed.errorMessage());
    TEST_ASSERT_EQUAL_UINT32(1U, parsed.deviceCount);
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(parsed.persistedStateBlobs.size()));

    DisplayLayoutRecordV1 layout{};
    TEST_ASSERT_TRUE(
        decodeDisplayLayoutBinary(parsed.persistedStateBlobs[0].blob.data(), parsed.persistedStateBlobs[0].blob.size(), layout));
    TEST_ASSERT_EQUAL_UINT32(10U, layout.deviceId);
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(layout.pages.size()));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(layout.pages[0].widgets.size()));
    TEST_ASSERT_EQUAL_STRING("Hello", layout.pages[0].widgets[0].text);
}

void test_device_setup_transfer_v3_rejects_next_device_before_layout_end() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":3,\"deviceCount\":2}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":10,\"typeName\":\"ssd1306\"},"
                               "\"config\":{\"name\":\"OLED\",\"enabled\":true,\"deps\":[{\"role\":\"i2c_bus\","
                               "\"deviceId\":1}],\"i2cAddress\":60}}\n"
                               "{\"kind\":\"layout_begin\",\"deviceId\":10,\"schemaVersion\":1,\"activePageId\":\"main\",\"pageCount\":1}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":11,\"typeName\":\"gpio_switch\"},"
                               "\"config\":{\"name\":\"Pump\",\"enabled\":true,\"gpioPin\":27}}\n";
    const char* path = "/tmp/device_setup_transfer_v3_incomplete_display.ndjson";
    writeTextFile(path, bundle);

    const DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_FALSE(parsed.ok());
    TEST_ASSERT_TRUE(std::string(parsed.errorMessage()).find("setup records are incomplete") != std::string::npos);
}

void test_device_setup_transfer_accepts_legacy_v2_embedded_display_layout() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":2,\"deviceCount\":1}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":10,\"typeName\":\"ssd1306\"},"
                               "\"config\":{\"name\":\"OLED\",\"enabled\":true,\"deps\":[{\"role\":\"i2c_bus\","
                               "\"deviceId\":1}],\"i2cAddress\":60,"
                               "\"layout\":{\"schemaVersion\":1,\"activePageId\":\"main\",\"pages\":[{\"id\":\"main\",\"name\":\"Main\","
                               "\"widgets\":[]}]}}}\n";
    const char* path = "/tmp/device_setup_transfer_v2_display.ndjson";
    writeTextFile(path, bundle);

    const DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE_MESSAGE(parsed.ok(), parsed.errorMessage());
    TEST_ASSERT_EQUAL_UINT32(1U, parsed.deviceCount);
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(parsed.persistedStateBlobs.size()));
}

void test_device_setup_transfer_v3_bounds_each_max_bitmap_record() {
    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{1001, 1002};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    const DeviceId busId = createFromJson(registry, "i2c_bus",
                                          R"({"config":{"name":"Main I2C","enabled":true,"sdaPin":21,"sclPin":22,"frequencyHz":400000}})");
    const std::string bitmapData(4096U, 'A');
    const std::string displayJson =
        std::string("{\"config\":{\"name\":\"OLED\",\"enabled\":true,\"deps\":[{\"role\":\"i2c_bus\",\"deviceId\":") +
        std::to_string(busId) +
        "}],\"i2cAddress\":60,\"layout\":{\"activePageId\":\"main\",\"pages\":[{\"id\":\"main\",\"name\":\"Main\","
        "\"widgets\":[{\"id\":\"image\",\"type\":\"bitmap\",\"width\":128,\"height\":64,\"bitmapFormat\":\"mono1\","
        "\"bitmapData\":\"" +
        bitmapData + "\"}] }]}}}";
    (void)createFromJson(registry, "ssd1306", displayJson.c_str());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, transferAdapters(), registry.registryRevision()));
    TEST_ASSERT_TRUE(bundle.find("\"kind\":\"layout_widget\"") != std::string::npos);

    size_t offset = 0U;
    while (offset < bundle.size()) {
        const size_t end = bundle.find('\n', offset);
        const size_t length = (end == std::string::npos ? bundle.size() : end) - offset;
        TEST_ASSERT_LESS_OR_EQUAL_UINT32(8192U, static_cast<uint32_t>(length));
        offset = end == std::string::npos ? bundle.size() : end + 1U;
    }

    const char* path = "/tmp/device_setup_transfer_v3_max_bitmap.ndjson";
    writeTextFile(path, bundle);
    const DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE_MESSAGE(parsed.ok(), parsed.errorMessage());
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(parsed.persistedStateBlobs.size()));
}

void test_device_setup_transfer_accepts_hand_written_minimal_bundle() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":2}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":4,\"typeName\":\"gpio_switch\"},"
                               "\"config\":{\"name\":\"Edited Pump\",\"enabled\":false,\"gpioPin\":27}}\n";
    const char* path = "/tmp/device_setup_transfer_minimal.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE_MESSAGE(parsed.ok(), parsed.errorMessage());
    TEST_ASSERT_EQUAL_UINT32(1U, parsed.deviceCount);

    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{9001};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.restore(parsed.snapshot, parsed.configBlobs, parsed.registryRevision, 0).ok());
    const IDeviceRuntime* runtime = registry.runtime(4);
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_STRING("Edited Pump", runtime->name());
}

void test_device_setup_transfer_reports_version_gap_for_old_config() {
    // ssd1306 is at config version >= 4; "layoutWidth" is a renamed v1-era field its parser
    // rejects, so the error must call out the version gap for hand fixing.
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":2}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":7,\"typeName\":\"ssd1306\",\"configVersion\":1},"
                               "\"config\":{\"name\":\"Old OLED\",\"enabled\":true,\"layoutWidth\":128}}\n";
    const char* path = "/tmp/device_setup_transfer_version_gap.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_FALSE(parsed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(parsed.validation.error));
    const std::string message = parsed.errorMessage();
    TEST_ASSERT_TRUE_MESSAGE(message.find("device 7 (ssd1306)") != std::string::npos, message.c_str());
    TEST_ASSERT_TRUE_MESSAGE(message.find("predates this firmware") != std::string::npos, message.c_str());
}

void test_device_setup_transfer_rejects_unknown_type_name() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":2}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":5,\"typeName\":\"warp_drive\"},"
                               "\"config\":{\"name\":\"Nope\",\"enabled\":true}}\n";
    const char* path = "/tmp/device_setup_transfer_unknown_type.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_FALSE(parsed.ok());
    const std::string message = parsed.errorMessage();
    TEST_ASSERT_TRUE_MESSAGE(message.find("unknown device type 'warp_drive'") != std::string::npos, message.c_str());
}

void test_device_setup_transfer_accepts_legacy_v1_bundle() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":1,\"registrySchemaVersion\":1,"
                               "\"registryRevision\":7,\"deviceCount\":1}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":1001,\"typeName\":\"dummy\",\"configRevision\":3},"
                               "\"config\":{\"name\":\"Legacy Lamp\",\"enabled\":true,\"deps\":[]}}\n";
    const char* path = "/tmp/device_setup_transfer_legacy_v1.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE_MESSAGE(parsed.ok(), parsed.errorMessage());
    TEST_ASSERT_EQUAL_UINT32(1U, parsed.deviceCount);
    TEST_ASSERT_EQUAL_UINT32(7U, parsed.registryRevision);
    TEST_ASSERT_TRUE(parsed.dashboardLayoutJson.empty());
    TEST_ASSERT_TRUE(parsed.persistedStateBlobs.empty());
}

void test_device_setup_transfer_captures_dashboard_layout_line() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":2}\n"
                               "{\"kind\":\"dashboard_layout\",\"revision\":3,\"layout\":{\"schema_version\":1,"
                               "\"active_panel_id\":\"main\",\"panels\":[]}}\n";
    const char* path = "/tmp/device_setup_transfer_dashboard.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE_MESSAGE(parsed.ok(), parsed.errorMessage());
    TEST_ASSERT_EQUAL_UINT32(0U, parsed.deviceCount);
    TEST_ASSERT_TRUE(parsed.dashboardLayoutJson.find("\"active_panel_id\":\"main\"") != std::string::npos);

    const size_t firstLineEnd = bundle.find('\n');
    TEST_ASSERT_TRUE(firstLineEnd != std::string::npos);
    const size_t dashboardLineStart = firstLineEnd + 1U;
    const size_t dashboardLineEnd = bundle.find('\n', dashboardLineStart);
    TEST_ASSERT_TRUE(dashboardLineEnd != std::string::npos);
    DynamicJsonDocument dashboardDoc(512);
    TEST_ASSERT_FALSE(deserializeJson(dashboardDoc, bundle.substr(dashboardLineStart, dashboardLineEnd - dashboardLineStart)));
    assertMatchesJsonSchema("schemas/rest/v1/bundle/dashboard-layout.schema.json", dashboardDoc.as<JsonVariantConst>());

    const std::string duplicate = bundle + "{\"kind\":\"dashboard_layout\",\"layout\":{\"panels\":[]}}\n";
    writeTextFile(path, duplicate);
    parsed = DeviceSetupTransferCodec::parseFile(path, duplicate.size(), transferAdapters());
    TEST_ASSERT_FALSE(parsed.ok());
}

void test_device_setup_transfer_rejects_device_count_mismatch() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":2,\"deviceCount\":2}\n"
                               "{\"kind\":\"device\",\"record\":{\"id\":4,\"typeName\":\"gpio_switch\"},"
                               "\"config\":{\"name\":\"Pump\",\"enabled\":true,\"gpioPin\":27}}\n";
    const char* path = "/tmp/device_setup_transfer_count_mismatch.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_FALSE(parsed.ok());
}

void test_device_setup_transfer_rejects_unsupported_version() {
    const std::string bundle = "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":99,\"registrySchemaVersion\":1,\"registry"
                               "Revision\":1,\"deviceCount\":0}\n";
    const char* path = "/tmp/device_setup_transfer_bad_version.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_FALSE(parsed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidVersion), static_cast<int>(parsed.validation.error));
}

void test_device_setup_transfer_rejects_legacy_flat_device_record() {
    const std::string bundle =
        "{\"kind\":\"transfer_envelope\",\"transferSchemaVersion\":1,\"registrySchemaVersion\":1,\"registryRevision\":1,"
        "\"deviceCount\":1}\n"
        "{\"kind\":\"device\",\"id\":1001,\"typeName\":\"dummy\",\"configRevision\":1,\"name\":\"legacy\",\"enabled\":true,"
        "\"config_blob_hex\":\"00\"}\n";
    const char* path = "/tmp/device_setup_transfer_flat_legacy.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_FALSE(parsed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(parsed.validation.error));
}

void test_device_setup_restore_failure_leaves_live_registry_unchanged() {
    MemoryConfigStorage storage;
    FixedDeviceIdSource idSource{1001};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore registryStore(storage);
    DeviceRegistry registry(registryStore, typeRegistry, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("Aquarium Lamp"), 0).ok());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, transferAdapters(), registry.registryRevision()));

    const char* path = "/tmp/device_setup_transfer_restore_failure.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE(parsed.ok());

    FailingStorage failingStorage;
    FixedDeviceIdSource failingIdSource{3001};
    DeviceRegistryStore failingRegistryStore(failingStorage);
    DeviceRegistry failingRegistry(failingRegistryStore, typeRegistry, failingIdSource);
    TEST_ASSERT_TRUE(failingRegistry.begin(0).ok());
    TEST_ASSERT_TRUE(failingRegistry.create(makeDummyCreateRequest("Existing Lamp"), 0).ok());
    failingStorage.failNextBlobWrite();

    const DeviceValidationResult restore = failingRegistry.restore(parsed.snapshot, parsed.configBlobs, parsed.registryRevision, 0);
    TEST_ASSERT_FALSE(restore.ok());
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(failingRegistry.list().size()));
    TEST_ASSERT_NOT_NULL(failingRegistry.runtime(3001));
    TEST_ASSERT_EQUAL_STRING("Existing Lamp", failingRegistry.runtime(3001)->name());
}

void test_device_setup_restore_rejects_persisted_state_before_swapping_registry() {
    MemoryConfigStorage sourceStorage;
    FixedDeviceIdSource sourceIdSource{1001};
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistryStore sourceRegistryStore(sourceStorage);
    DeviceRegistry source(sourceRegistryStore, typeRegistry, sourceIdSource);
    TEST_ASSERT_TRUE(source.begin(0).ok());
    TEST_ASSERT_TRUE(source.create(makeDummyCreateRequest("Imported Lamp"), 0).ok());

    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, source, transferAdapters(), source.registryRevision()));
    const char* path = "/tmp/device_setup_transfer_invalid_state.ndjson";
    writeTextFile(path, bundle);
    const DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), transferAdapters());
    TEST_ASSERT_TRUE(parsed.ok());

    MemoryConfigStorage targetStorage;
    FixedDeviceIdSource targetIdSource{3001};
    DeviceRegistryStore targetRegistryStore(targetStorage);
    DeviceRegistry target(targetRegistryStore, typeRegistry, targetIdSource);
    TEST_ASSERT_TRUE(target.begin(0).ok());
    TEST_ASSERT_TRUE(target.create(makeDummyCreateRequest("Existing Lamp"), 0).ok());

    const uint8_t invalidState[] = {1U};
    const std::vector<DevicePersistedStateUpdate> updates{{1001U, invalidState, sizeof(invalidState)}};
    const DeviceValidationResult restore = target.restore(parsed.snapshot, parsed.configBlobs, updates, parsed.registryRevision, 0);
    TEST_ASSERT_FALSE(restore.ok());
    TEST_ASSERT_NOT_NULL(target.runtime(3001));
    TEST_ASSERT_NULL(target.runtime(1001));
    TEST_ASSERT_EQUAL_STRING("Existing Lamp", target.runtime(3001)->name());
}
