#include "config/MemoryConfigStorage.h"
#include "devices/bus/spi/SpiBusConfig.h"
#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/core/ConfigCodec.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/st7735/St7735Device.h"
#include "devices/display/st7735/St7735DeviceConfig.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/spi_bus/SpiBusDeviceApiAdapter.h"
#include "integrations/rest/st7735/St7735DeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>
#include <unity.h>

using namespace ewfm;

namespace {

St7735DeviceConfigV4 makeConfig(uint32_t spiBusDeviceId = 12, uint8_t chipSelectPin = 5, uint8_t dcPin = 2, int8_t resetPin = -1,
                                uint8_t rotation = 1U, uint16_t width = 128, uint16_t height = 160) {
    St7735DeviceConfigV4 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "st7735");
    config.spiBusDeviceId = spiBusDeviceId;
    config.chipSelectPin = chipSelectPin;
    config.dcPin = dcPin;
    config.resetPin = resetPin;
    config.rotation = rotation;
    config.width = width;
    config.height = height;
    return config;
}

void fillSpiBusDocument(StaticJsonDocument<512>& doc, const char* name, uint8_t sckPin, uint8_t mosiPin, int16_t misoPin) {
    doc.clear();
    doc["typeName"] = "spi_bus";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = name;
    config["enabled"] = true;
    config["host"] = 2U;
    config["sckPin"] = sckPin;
    config["mosiPin"] = mosiPin;
    config["misoPin"] = misoPin;
}

void fillDisplayDocument(StaticJsonDocument<1024>& doc, uint32_t spiBusDeviceId, uint8_t chipSelectPin, bool includeLayout) {
    doc.clear();
    doc["typeName"] = "st7735";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "st7735";
    config["enabled"] = true;
    config["spiBusDeviceId"] = spiBusDeviceId;
    config["chipSelectPin"] = chipSelectPin;
    config["dcPin"] = 2;
    config["resetPin"] = -1;
    config["rotation"] = 1;
    config["width"] = 128;
    config["height"] = 160;
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
    widget["id"] = "bitmap";
    widget["type"] = "bitmap";
    widget["bindingKind"] = static_cast<uint8_t>(DisplayLayoutBindingKind::Unbound);
    widget["x"] = 0;
    widget["y"] = 0;
    widget["width"] = 2;
    widget["height"] = 2;
    widget["bitmapFormat"] = "rgb565";
    widget["keepAspectRatio"] = true;
    widget["bitmapData"] = "AAECAw==";
}

DeviceCreateRequest makeCreateRequest(uint32_t spiBusDeviceId, uint8_t chipSelectPin) {
    StaticJsonDocument<1024> doc;
    fillDisplayDocument(doc, spiBusDeviceId, chipSelectPin, false);
    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(St7735DeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_NULL(error);
    return request;
}

} // namespace

void test_st7735_config_codec_round_trip() {
    const St7735DeviceConfigV4 config = makeConfig();
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(St7735DeviceConfigV4::kMagic, config, buffer, st7735DeviceConfigSize(config)));

    St7735DeviceConfigV4 decoded{};
    TEST_ASSERT_TRUE(decodeSt7735DeviceConfig(buffer, st7735DeviceConfigSize(config), decoded));
    TEST_ASSERT_EQUAL_UINT32(config.spiBusDeviceId, decoded.spiBusDeviceId);
    TEST_ASSERT_EQUAL_UINT8(config.chipSelectPin, decoded.chipSelectPin);
    TEST_ASSERT_EQUAL_UINT8(config.dcPin, decoded.dcPin);
    TEST_ASSERT_EQUAL_INT8(config.resetPin, decoded.resetPin);
    TEST_ASSERT_EQUAL_UINT8(config.rotation, decoded.rotation);
    TEST_ASSERT_EQUAL_UINT16(config.width, decoded.width);
    TEST_ASSERT_EQUAL_UINT16(config.height, decoded.height);
}

void test_st7735_config_codec_accepts_legacy_blob() {
    const St7735DeviceConfigV4 config = makeConfig();
    EWFM_LEGACY_CONFIG_USE_BEGIN
    St7735DeviceConfigV3 legacyV3{};
    legacyV3.enabled = config.enabled;
    std::memcpy(legacyV3.name, config.name, sizeof(legacyV3.name));
    legacyV3.spiBusDeviceId = config.spiBusDeviceId;
    legacyV3.chipSelectPin = config.chipSelectPin;
    legacyV3.dcPin = config.dcPin;
    legacyV3.resetPin = config.resetPin;
    legacyV3.width = config.width;
    legacyV3.height = config.height;
    uint8_t legacyV3Buffer[kMaxDeviceConfigBytes]{};
    const size_t legacyV3Size = sizeof(St7735DeviceConfigV3::kMagic) - 1U + sizeof(St7735DeviceConfigV3);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(St7735DeviceConfigV3::kMagic, legacyV3, legacyV3Buffer, legacyV3Size));
    EWFM_LEGACY_CONFIG_USE_END

    St7735DeviceConfigV4 decodedV3{};
    TEST_ASSERT_TRUE(decodeSt7735DeviceConfig(legacyV3Buffer, legacyV3Size, decodedV3));
    TEST_ASSERT_EQUAL_UINT8(0U, decodedV3.rotation);

    EWFM_LEGACY_CONFIG_USE_BEGIN
    St7735DeviceConfigV1 legacy{};
    legacy.enabled = config.enabled;
    std::memcpy(legacy.name, config.name, sizeof(legacy.name));
    legacy.spiBusDeviceId = config.spiBusDeviceId;
    legacy.chipSelectPin = config.chipSelectPin;
    legacy.layoutWidth = config.width;
    legacy.layoutHeight = config.height;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(St7735DeviceConfigV1::kMagic, legacy, buffer, st7735DeviceConfigV1Size()));
    EWFM_LEGACY_CONFIG_USE_END

    St7735DeviceConfigV4 decoded{};
    TEST_ASSERT_TRUE(decodeSt7735DeviceConfig(buffer, st7735DeviceConfigV1Size(), decoded));
    TEST_ASSERT_EQUAL_UINT32(config.spiBusDeviceId, decoded.spiBusDeviceId);
    TEST_ASSERT_EQUAL_UINT8(config.chipSelectPin, decoded.chipSelectPin);
    TEST_ASSERT_EQUAL_UINT8(2U, decoded.dcPin);
    TEST_ASSERT_EQUAL_INT8(-1, decoded.resetPin);
    TEST_ASSERT_EQUAL_UINT8(0U, decoded.rotation);
    TEST_ASSERT_EQUAL_UINT16(config.width, decoded.width);
    TEST_ASSERT_EQUAL_UINT16(config.height, decoded.height);
}

void test_st7735_config_codec_migrates_v2_blob() {
    const St7735DeviceConfigV4 config = makeConfig(12, 5, 4, 3, 1U, 128, 160);
    EWFM_LEGACY_CONFIG_USE_BEGIN
    St7735DeviceConfigV2 legacy{};
    legacy.enabled = config.enabled;
    std::memcpy(legacy.name, config.name, sizeof(legacy.name));
    legacy.spiBusDeviceId = config.spiBusDeviceId;
    legacy.chipSelectPin = config.chipSelectPin;
    legacy.dcPin = config.dcPin;
    legacy.resetPin = config.resetPin;
    legacy.layoutWidth = config.width;
    legacy.layoutHeight = config.height;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(St7735DeviceConfigV2::kMagic, legacy, buffer, st7735DeviceConfigV2Size()));
    EWFM_LEGACY_CONFIG_USE_END

    St7735DeviceConfigV4 decoded{};
    TEST_ASSERT_TRUE(decodeSt7735DeviceConfig(buffer, st7735DeviceConfigV2Size(), decoded));
    TEST_ASSERT_EQUAL_UINT32(config.spiBusDeviceId, decoded.spiBusDeviceId);
    TEST_ASSERT_EQUAL_UINT8(config.chipSelectPin, decoded.chipSelectPin);
    TEST_ASSERT_EQUAL_UINT8(config.dcPin, decoded.dcPin);
    TEST_ASSERT_EQUAL_INT8(config.resetPin, decoded.resetPin);
    TEST_ASSERT_EQUAL_UINT8(0U, decoded.rotation);
    TEST_ASSERT_EQUAL_UINT16(config.width, decoded.width);
    TEST_ASSERT_EQUAL_UINT16(config.height, decoded.height);
}

void test_st7735_config_rejects_legacy_layout_dimension_fields() {
    StaticJsonDocument<1024> doc;
    fillDisplayDocument(doc, 12U, 5U, false);
    JsonObject config = doc["config"].as<JsonObject>();
    config["layoutHeight"] = 160;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(St7735DeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_NOT_NULL(error);
}

void test_st7735_registry_migrates_legacy_blob_on_begin() {
    MemoryConfigStorage registryStorage;
    MemoryConfigStorage retainedStorage;
    MemoryConfigStorage scopedStorage;
    DeviceRegistryStore registryStore(registryStorage);
    DeviceRetainedDataStore retainedStore(retainedStorage);
    DeviceScopedDataStore scopedStore(scopedStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    TEST_ASSERT_TRUE(scopedStore.begin(false));

    DeviceRegistrySnapshot snapshot{};
    DeviceConfigBlobMap configBlobs{};

    SpiBusDeviceConfigV1 spiConfig{};
    spiConfig.enabled = 1U;
    std::snprintf(spiConfig.name, sizeof(spiConfig.name), "%s", "spi");
    spiConfig.host = kSpiBusHostVspi;
    spiConfig.sckPin = 18U;
    spiConfig.mosiPin = 23U;
    spiConfig.misoPin = -1;

    DeviceRegistryEntry spiRecord{};
    spiRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    spiRecord.header.deviceId = 10U;
    spiRecord.header.typeId = SpiBusDevice::descriptor().typeId;
    spiRecord.header.configVersion = SpiBusDevice::descriptor().currentConfigVersion;
    spiRecord.header.configRevision = 1U;
    spiRecord.depCount = 0U;
    spiRecord.persistencePolicy = DevicePersistencePolicy::Delayed;
    spiRecord.status = DeviceStatus::Ready;

    uint8_t spiBuffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(SpiBusDeviceConfigV1::kMagic, spiConfig, spiBuffer, spiBusDeviceConfigSize(spiConfig)));
    BoundedBlob<kMaxDeviceConfigBytes> spiBlob{};
    TEST_ASSERT_TRUE(spiBlob.assign(spiBuffer, spiBusDeviceConfigSize(spiConfig)));

    St7735DeviceConfigV4 legacyDisplayConfig = makeConfig(spiRecord.header.deviceId);
    uint8_t legacyBuffer[kMaxDeviceConfigBytes]{};
    EWFM_LEGACY_CONFIG_USE_BEGIN
    St7735DeviceConfigV1 legacy{};
    legacy.enabled = legacyDisplayConfig.enabled;
    std::memcpy(legacy.name, legacyDisplayConfig.name, sizeof(legacy.name));
    legacy.spiBusDeviceId = legacyDisplayConfig.spiBusDeviceId;
    legacy.chipSelectPin = legacyDisplayConfig.chipSelectPin;
    legacy.layoutWidth = legacyDisplayConfig.width;
    legacy.layoutHeight = legacyDisplayConfig.height;
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(St7735DeviceConfigV1::kMagic, legacy, legacyBuffer, st7735DeviceConfigV1Size()));
    EWFM_LEGACY_CONFIG_USE_END

    DeviceRegistryEntry displayRecord{};
    displayRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    displayRecord.header.deviceId = 11U;
    displayRecord.header.typeId = St7735Device::descriptor().typeId;
    displayRecord.header.configVersion = 1U;
    displayRecord.header.configRevision = 4U;
    displayRecord.depCount = 1U;
    displayRecord.deps[0] = {DeviceRole::SpiBus, spiRecord.header.deviceId};
    displayRecord.persistencePolicy = DevicePersistencePolicy::Delayed;
    displayRecord.status = DeviceStatus::Ready;

    snapshot.records.push_back(spiRecord);
    snapshot.records.push_back(displayRecord);
    snapshot.indexEntries.push_back({spiRecord.header.deviceId, spiRecord.header.typeId});
    snapshot.indexEntries.push_back({displayRecord.header.deviceId, displayRecord.header.typeId});
    configBlobs[spiRecord.header.deviceId] = spiBlob;
    configBlobs[displayRecord.header.deviceId] = BoundedBlob<kMaxDeviceConfigBytes>{};
    TEST_ASSERT_TRUE(configBlobs[displayRecord.header.deviceId].assign(legacyBuffer, st7735DeviceConfigV1Size()));

    TEST_ASSERT_TRUE(registryStore.save(snapshot, configBlobs).ok());

    SequentialDeviceIdSource idSource(1000);
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, typeRegistry, idSource, &retainedStore, &scopedStore);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    const St7735Device* runtime = dynamic_cast<const St7735Device*>(registry.runtime(displayRecord.header.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_UINT32(St7735Device::descriptor().currentConfigVersion, runtime->configVersion());
    TEST_ASSERT_EQUAL_UINT8(2U, runtime->config().dcPin);
    TEST_ASSERT_EQUAL_INT8(-1, runtime->config().resetPin);
    TEST_ASSERT_EQUAL_UINT8(0U, runtime->config().rotation);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
}

void test_st7735_api_adapter_partial_update_preserves_bus_and_pins() {
    St7735DeviceConfigV4 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "st7735");
    config.spiBusDeviceId = 12;
    // chipSelectPin/dcPin/width/height are deliberately non-default (compiled defaults are
    // 5/2/128/160) so these assertions cannot pass by accident if the merge fix regresses and the
    // fields are silently reset to their struct defaults instead of the runtime's current values.
    config.chipSelectPin = 15;
    config.dcPin = 16;
    config.resetPin = 4;
    config.rotation = 0;
    config.width = 100;
    config.height = 120;

    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = st7735DeviceConfigSize(config);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(St7735DeviceConfigV4::kMagic, config, buffer, size));
    DeviceConfigBlob configBlob{};
    TEST_ASSERT_TRUE(configBlob.assign(buffer, size));

    DeviceRegistryEntry record{};
    record.header.deviceId = 200;
    record.header.typeId = St7735Device::descriptor().typeId;
    record.header.configVersion = St7735Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::SpiBus, 12};
    record.status = DeviceStatus::Ready;

    St7735Device device(record, configBlob);

    StaticJsonDocument<128> doc;
    JsonObject updateConfig = doc.createNestedObject("config");
    updateConfig["rotation"] = 1;
    // Decoy value: the spi bus link is authoritatively taken from the runtime's current
    // dependency (12), never from this config-body field, even though it is present in the JSON.
    updateConfig["spiBusDeviceId"] = 99;

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(St7735DeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), device, request, error),
                             error);
    TEST_ASSERT_FALSE_MESSAGE(request.depsProvided, "depsProvided must stay false when the update carries no multi-source layout");
    TEST_ASSERT_EQUAL_UINT8(1U, request.depCount);
    TEST_ASSERT_EQUAL_UINT32(12U, request.deps[0].deviceId);

    St7735DeviceConfigV4 parsed{};
    TEST_ASSERT_TRUE(
        decodeSt7735DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(1, parsed.rotation);
    TEST_ASSERT_EQUAL_UINT8(15, parsed.chipSelectPin);
    TEST_ASSERT_EQUAL_UINT8(16, parsed.dcPin);
    TEST_ASSERT_EQUAL_INT8(4, parsed.resetPin);
    TEST_ASSERT_EQUAL_UINT16(100, parsed.width);
    TEST_ASSERT_EQUAL_UINT16(120, parsed.height);
}

void test_st7735_default_registries_include_display() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(typeRegistry.find(St7735Device::descriptor().typeId));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(St7735Device::descriptor().typeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("st7735"));
}

void test_st7735_requires_spi_bus_and_rejects_duplicate_chip_select() {
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
    fillSpiBusDocument(busDoc, "spi", 18U, 23U, -1);
    DeviceCreateRequest busRequest{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(SpiBusDeviceApiAdapter::instance().parseCreateRequest(busDoc.as<JsonObjectConst>(), busRequest, error));
    TEST_ASSERT_NULL(error);
    const DeviceCreateResult busResult = registry.create(busRequest, 0);
    TEST_ASSERT_TRUE(busResult.ok());

    DeviceCreateRequest createRequest = makeCreateRequest(busResult.deviceId, 5U);
    const DeviceValidationResult createValidation = St7735DeviceApiAdapter::instance().validateCreateRequest(createRequest, registry);
    TEST_ASSERT_TRUE(createValidation.ok());
    const DeviceCreateResult createResult = registry.create(createRequest, 0);
    TEST_ASSERT_TRUE(createResult.ok());

    DeviceCreateRequest duplicateRequest = makeCreateRequest(busResult.deviceId, 5U);
    const DeviceValidationResult duplicateValidation = St7735DeviceApiAdapter::instance().validateCreateRequest(duplicateRequest, registry);
    TEST_ASSERT_FALSE(duplicateValidation.ok());
    TEST_ASSERT_EQUAL(DeviceError::InvalidRelationship, duplicateValidation.error);
}

void test_st7735_update_round_trip_includes_layout() {
    MemoryConfigStorage registryStorage;
    MemoryConfigStorage retainedStorage;
    MemoryConfigStorage scopedStorage;
    DeviceRegistryStore registryStore(registryStorage);
    DeviceRetainedDataStore retainedStore(retainedStorage);
    DeviceScopedDataStore scopedStore(scopedStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    TEST_ASSERT_TRUE(scopedStore.begin(false));

    SequentialDeviceIdSource idSource(200);
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, typeRegistry, idSource, &retainedStore, &scopedStore);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    StaticJsonDocument<512> busDoc;
    fillSpiBusDocument(busDoc, "spi", 18U, 23U, -1);
    DeviceCreateRequest busRequest{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(SpiBusDeviceApiAdapter::instance().parseCreateRequest(busDoc.as<JsonObjectConst>(), busRequest, error));
    TEST_ASSERT_NULL(error);
    const DeviceCreateResult busResult = registry.create(busRequest, 0);
    TEST_ASSERT_TRUE(busResult.ok());

    StaticJsonDocument<1024> createDoc;
    fillDisplayDocument(createDoc, busResult.deviceId, 5U, false);
    DeviceCreateRequest createRequest{};
    TEST_ASSERT_TRUE(St7735DeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), createRequest, error));
    TEST_ASSERT_NULL(error);
    const DeviceCreateResult createResult = registry.create(createRequest, 0);
    TEST_ASSERT_TRUE(createResult.ok());

    IDeviceRuntime* runtime = registry.runtime(createResult.deviceId);
    TEST_ASSERT_NOT_NULL(runtime);

    StaticJsonDocument<1024> updateDoc;
    fillDisplayDocument(updateDoc, busResult.deviceId, 5U, true);
    DeviceConfigUpdateRequest updateRequest{};
    TEST_ASSERT_TRUE(
        St7735DeviceApiAdapter::instance().parseUpdateConfigRequest(updateDoc.as<JsonObjectConst>(), *runtime, updateRequest, error));
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

    const St7735Device* reloadedRuntime = dynamic_cast<const St7735Device*>(registry.runtime(createResult.deviceId));
    TEST_ASSERT_NOT_NULL(reloadedRuntime);
    TEST_ASSERT_EQUAL_UINT32(busResult.deviceId, reloadedRuntime->config().spiBusDeviceId);
    TEST_ASSERT_EQUAL_UINT8(5U, reloadedRuntime->config().chipSelectPin);
    TEST_ASSERT_EQUAL_UINT8(2U, reloadedRuntime->config().dcPin);
    TEST_ASSERT_EQUAL_INT8(-1, reloadedRuntime->config().resetPin);
    TEST_ASSERT_EQUAL_UINT8(1U, reloadedRuntime->layout().pages.size());
    TEST_ASSERT_EQUAL_UINT8(1U, reloadedRuntime->layout().pages[0].widgets.size());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutWidgetType::Bitmap), reloadedRuntime->layout().pages[0].widgets[0].type);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DisplayLayoutBitmapFormat::Rgb565),
                            reloadedRuntime->layout().pages[0].widgets[0].bitmapFormat);
    TEST_ASSERT_EQUAL_UINT8(1U, reloadedRuntime->layout().pages[0].widgets[0].keepAspectRatio);
    TEST_ASSERT_EQUAL_UINT8(4U, reloadedRuntime->layout().pages[0].widgets[0].bitmapData.size());

    StaticJsonDocument<2048> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    St7735DeviceApiAdapter::instance().writeDeviceJson(*reloadedRuntime, reloadedRuntime->status(), output);
    TEST_ASSERT_EQUAL_UINT16(128U, output["config"]["width"].as<uint16_t>());
    TEST_ASSERT_EQUAL_UINT16(160U, output["config"]["height"].as<uint16_t>());
    TEST_ASSERT_EQUAL_UINT8(2U, output["config"]["dcPin"].as<uint8_t>());
    TEST_ASSERT_EQUAL_INT8(-1, output["config"]["resetPin"].as<int8_t>());
}
