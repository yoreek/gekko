#include "config/MemoryConfigStorage.h"
#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
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
#include <unity.h>

using namespace ewfm;

namespace {

St7735DeviceConfigV1 makeConfig(uint32_t spiBusDeviceId = 12, uint8_t chipSelectPin = 5, uint16_t width = 128, uint16_t height = 160) {
    St7735DeviceConfigV1 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "st7735");
    config.spiBusDeviceId = spiBusDeviceId;
    config.chipSelectPin = chipSelectPin;
    config.layoutWidth = width;
    config.layoutHeight = height;
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
    config["layoutWidth"] = 128;
    config["layoutHeight"] = 160;
    if (!includeLayout) {
        return;
    }
    JsonObject layout = config.createNestedObject("layout");
    layout["schemaVersion"] = 1;
    layout["activePageId"] = "main";
    JsonArray pages = layout.createNestedArray("pages");
    JsonObject page = pages.createNestedObject();
    page["id"] = "main";
    page.createNestedArray("widgets");
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
    const St7735DeviceConfigV1 config = makeConfig();
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeSt7735DeviceConfig(config, buffer, st7735DeviceConfigSize(config)));

    St7735DeviceConfigV1 decoded{};
    TEST_ASSERT_TRUE(decodeSt7735DeviceConfig(buffer, st7735DeviceConfigSize(config), decoded));
    TEST_ASSERT_EQUAL_UINT32(config.spiBusDeviceId, decoded.spiBusDeviceId);
    TEST_ASSERT_EQUAL_UINT8(config.chipSelectPin, decoded.chipSelectPin);
    TEST_ASSERT_EQUAL_UINT16(config.layoutWidth, decoded.layoutWidth);
    TEST_ASSERT_EQUAL_UINT16(config.layoutHeight, decoded.layoutHeight);
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
        registry.updateConfigAndDeps(createResult.deviceId, updateRequest.configBlob, updateRequest.configVersion,
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
    TEST_ASSERT_EQUAL_UINT8(1U, reloadedRuntime->layout().pages.size());
}
