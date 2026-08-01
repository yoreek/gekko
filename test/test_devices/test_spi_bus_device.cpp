#include "JsonSchemaSmokeValidator.h"
#include "config/MemoryConfigStorage.h"
#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceSetupTransferCodec.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/spi_bus/SpiBusDeviceApiAdapter.h"

#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <string>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}

class FakeSpiBusDriver final : public ISpiBusDriver {
public:
    bool begin(uint8_t host, uint8_t sckPin, uint8_t mosiPin, uint8_t misoPin) override {
        began = true;
        ++beginCount;
        lastHost = host;
        lastSckPin = sckPin;
        lastMosiPin = mosiPin;
        lastMisoPin = misoPin;
        return beginOk;
    }

    bool end() override {
        ended = true;
        ++endCount;
        transactionActive = false;
        return endOk;
    }

    bool beginTransaction(uint32_t clockHz, uint8_t dataMode, uint8_t bitOrder) override {
        ++beginTransactionCount;
        lastClockHz = clockHz;
        lastDataMode = dataMode;
        lastBitOrder = bitOrder;
        transactionActive = beginTransactionOk;
        return beginTransactionOk;
    }

    void endTransaction() override {
        ++endTransactionCount;
        transactionActive = false;
    }

    uint8_t transfer(uint8_t data) override {
        lastTransfer = data;
        return data;
    }

    size_t transferBytes(const uint8_t* txData, uint8_t* rxData, size_t length) override {
        ++transferBytesCount;
        for (size_t index = 0; index < length; ++index) {
            const uint8_t value = txData != nullptr ? txData[index] : 0xFFU;
            if (rxData != nullptr) {
                rxData[index] = value;
            }
        }
        return length;
    }

    bool beginOk{true};
    bool endOk{true};
    bool beginTransactionOk{true};
    bool began{false};
    bool ended{false};
    bool transactionActive{false};
    uint32_t beginCount{0};
    uint32_t endCount{0};
    uint32_t beginTransactionCount{0};
    uint32_t endTransactionCount{0};
    uint32_t transferBytesCount{0};
    uint8_t lastHost{0};
    uint8_t lastSckPin{0};
    uint8_t lastMosiPin{0};
    uint8_t lastMisoPin{kGpioPinUnset};
    uint32_t lastClockHz{0};
    uint8_t lastDataMode{0};
    uint8_t lastBitOrder{0};
    uint8_t lastTransfer{0};
};

class FakeSpiDependentRuntime final : public DeviceRuntimeBase {
public:
    explicit FakeSpiDependentRuntime(uint8_t csPin) : DeviceRuntimeBase((PState)&FakeSpiDependentRuntime::Idle), csPin_(csPin) {}

    bool spiChipSelectPin(uint8_t& pin) const override {
        pin = csPin_;
        return true;
    }

    bool handleCommand(const DeviceCommand&) override {
        return false;
    }

private:
    State Idle();
    uint8_t csPin_{0};
};

#undef SM_CLASS
#define SM_CLASS FakeSpiDependentRuntime
SM_STATE(FakeSpiDependentRuntime::Idle) {
    status_ = DeviceStatus::Ready;
}
#undef SM_CLASS

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

SpiBusDeviceConfigV2 makeConfig(uint8_t host = kSpiBusHostVspi, uint8_t sckPin = 18U, uint8_t mosiPin = 23U,
                                uint8_t misoPin = kGpioPinUnset) {
    SpiBusDeviceConfigV2 config{};
    config.enabled = 1U;
    std::snprintf(config.name, sizeof(config.name), "%s", "spi-bus");
    config.host = host;
    config.sckPin = sckPin;
    config.mosiPin = mosiPin;
    config.misoPin = misoPin;
    return config;
}

BoundedBlob<kMaxDeviceConfigBytes> encodePayload(const SpiBusDeviceConfigV2& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(SpiBusDeviceConfigV2::kMagic, config, buffer, spiBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, spiBusDeviceConfigSize(config)));
    return payload;
}

void driveBusToReady(SpiBusDevice& bus, uint32_t startNow = 10U) {
    bus.begin(startNow);
    bus.tick100ms(startNow + 1U);
}

void bindBusIdentity(SpiBusDevice& bus, DeviceId busId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = busId;
    record.header.typeId = SpiBusDevice::descriptor().typeId;
    record.header.configVersion = SpiBusDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodePayload(bus.config()).size());
    record.status = DeviceStatus::Ready;
    bus.bindDeviceIdentity(record, encodePayload(bus.config()));
}

void writeTextFile(const char* path, const std::string& text) {
    std::ofstream file(path, std::ios::trunc);
    TEST_ASSERT_TRUE(file.is_open());
    file << text;
    file.close();
}

DeviceCreateRequest makeCreateRequest() {
    SpiBusDeviceConfigV2 config = makeConfig();
    DeviceCreateRequest request{};
    request.typeId = SpiBusDevice::descriptor().typeId;
    request.configVersion = SpiBusDevice::descriptor().currentConfigVersion;
    request.setEnabled(true);
    TEST_ASSERT_TRUE(request.assignName(config.name));
    request.configBlob = encodePayload(config);
    return request;
}

} // namespace

void test_spi_bus_config_codec_json_and_validation() {
    const SpiBusDeviceConfigV2 config = makeConfig(kSpiBusHostHspi, 18U, 23U, kGpioPinUnset);
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodePayload(config);

    SpiBusDeviceConfigV2 decoded{};
    TEST_ASSERT_TRUE(decodeSpiBusDeviceConfig(payload.data(), payload.size(), decoded));
    TEST_ASSERT_EQUAL_UINT8(config.host, decoded.host);
    TEST_ASSERT_EQUAL_UINT8(config.sckPin, decoded.sckPin);
    TEST_ASSERT_EQUAL_UINT8(config.mosiPin, decoded.mosiPin);
    TEST_ASSERT_EQUAL_UINT8(config.misoPin, decoded.misoPin);

    StaticJsonDocument<256> doc;
    JsonObject json = doc.to<JsonObject>();
    config.writeJson(json);
    assertMatchesJsonSchema("schemas/rest/v1/devices/spi_bus.config.schema.json", doc.as<JsonVariantConst>());

    SpiBusDeviceConfigV2 parsed{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(parsed.parseJson(json, error));
    TEST_ASSERT_TRUE(parsed.validate().ok());

    SpiBusDeviceConfigV2 invalidHost = config;
    invalidHost.host = 9U;
    TEST_ASSERT_FALSE(invalidHost.validate().ok());

    SpiBusDeviceConfigV2 invalidPins = config;
    invalidPins.sckPin = invalidPins.mosiPin;
    TEST_ASSERT_FALSE(invalidPins.validate().ok());
}

void test_spi_bus_config_migrates_legacy_v1_blob() {
    EWFM_LEGACY_CONFIG_USE_BEGIN
    SpiBusDeviceConfigV1 legacy{};
    std::snprintf(legacy.name, sizeof(legacy.name), "%s", "legacy-spi");
    legacy.enabled = 1U;
    legacy.host = kSpiBusHostHspi;
    legacy.sckPin = 14U;
    legacy.mosiPin = 13U;
    legacy.misoPin = 27; // real pin, was int16_t
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = spiBusDeviceConfigSize(legacy);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(SpiBusDeviceConfigV1::kMagic, legacy, buffer, size));
    EWFM_LEGACY_CONFIG_USE_END

    SpiBusDeviceConfigV2 migrated{};
    TEST_ASSERT_TRUE(decodeSpiBusDeviceConfig(buffer, size, migrated));
    TEST_ASSERT_EQUAL_STRING("legacy-spi", migrated.name);
    TEST_ASSERT_EQUAL_UINT8(kSpiBusHostHspi, migrated.host);
    TEST_ASSERT_EQUAL_UINT8(14U, migrated.sckPin);
    TEST_ASSERT_EQUAL_UINT8(13U, migrated.mosiPin);
    TEST_ASSERT_EQUAL_UINT8(27U, migrated.misoPin);

    EWFM_LEGACY_CONFIG_USE_BEGIN
    SpiBusDeviceConfigV1 legacyUnset{};
    std::snprintf(legacyUnset.name, sizeof(legacyUnset.name), "%s", "legacy-spi-unset");
    legacyUnset.enabled = 1U;
    legacyUnset.host = kSpiBusHostVspi;
    legacyUnset.sckPin = 18U;
    legacyUnset.mosiPin = 23U;
    legacyUnset.misoPin = -1;
    uint8_t bufferUnset[kMaxDeviceConfigBytes]{};
    const size_t sizeUnset = spiBusDeviceConfigSize(legacyUnset);
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(SpiBusDeviceConfigV1::kMagic, legacyUnset, bufferUnset, sizeUnset));
    EWFM_LEGACY_CONFIG_USE_END

    SpiBusDeviceConfigV2 migratedUnset{};
    TEST_ASSERT_TRUE(decodeSpiBusDeviceConfig(bufferUnset, sizeUnset, migratedUnset));
    TEST_ASSERT_EQUAL_UINT8(kGpioPinUnset, migratedUnset.misoPin);
}

void test_spi_bus_api_adapter_schema_smoke_and_runtime_serialization() {
    FakeSpiBusDriver driver;
    SpiBusDevice bus(makeConfig(), driver);
    driveBusToReady(bus);
    bindBusIdentity(bus, 2001);

    StaticJsonDocument<512> createDoc;
    JsonObject createJson = createDoc.to<JsonObject>();
    createJson["typeName"] = "spi_bus";
    JsonObject createConfig = createJson.createNestedObject("config");
    makeConfig().writeJson(createConfig);
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-spi_bus.request.schema.json", createDoc.as<JsonVariantConst>());

    DeviceCreateRequest createRequest{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(SpiBusDeviceApiAdapter::instance().parseCreateRequest(createDoc.as<JsonObjectConst>(), createRequest, error),
                             error);

    StaticJsonDocument<256> updateDoc;
    JsonObject updateJson = updateDoc.to<JsonObject>();
    JsonObject updateConfig = updateJson.createNestedObject("config");
    updateConfig["misoPin"] = 27;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-spi_bus.request.schema.json", updateDoc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest updateRequest{};
    TEST_ASSERT_TRUE_MESSAGE(
        SpiBusDeviceApiAdapter::instance().parseUpdateConfigRequest(updateDoc.as<JsonObjectConst>(), bus, updateRequest, error), error);

    StaticJsonDocument<768> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    SpiBusDeviceApiAdapter::instance().writeDeviceJson(bus, bus.status(), output);
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-spi_bus.response.schema.json", outputDoc.as<JsonVariantConst>());
}

void test_spi_bus_api_adapter_partial_update_preserves_pins() {
    StaticJsonDocument<128> doc;
    JsonObject config = doc.createNestedObject("config");
    config["misoPin"] = 27;

    // sckPin/mosiPin are deliberately non-default (compiled defaults are 18/23) so these
    // assertions cannot pass by accident if the merge fix regresses and the fields are silently
    // reset to their struct defaults instead of the runtime's current pins.
    FakeSpiBusDriver driver;
    SpiBusDevice runtime(makeConfig(kSpiBusHostHspi, 14U, 13U, kGpioPinUnset), driver);

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        SpiBusDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error), error);

    SpiBusDeviceConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodeSpiBusDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(27U, parsed.misoPin);
    TEST_ASSERT_EQUAL_UINT8(kSpiBusHostHspi, parsed.host);
    TEST_ASSERT_EQUAL_UINT8(14U, parsed.sckPin);
    TEST_ASSERT_EQUAL_UINT8(13U, parsed.mosiPin);
}

void test_spi_default_registries_include_bus() {
    DeviceTypeRegistry typeRegistry = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(typeRegistry.find(SpiBusDevice::descriptor().typeId));

    DeviceApiAdapterRegistry adapterRegistry = DeviceApiAdapterRegistry::withDefaults();
    TEST_ASSERT_NOT_NULL(adapterRegistry.find(SpiBusDevice::descriptor().typeId));
    TEST_ASSERT_NOT_NULL(adapterRegistry.findByName("spi_bus"));
}

void test_spi_runtime_lifecycle_transactions_and_duplicate_cs_detection() {
    FakeSpiBusDriver driver;
    SpiBusDevice bus(makeConfig(), driver);
    driveBusToReady(bus);

    TEST_ASSERT_TRUE(driver.began);
    TEST_ASSERT_EQUAL_UINT8(kSpiBusHostVspi, driver.lastHost);
    TEST_ASSERT_EQUAL_UINT8(18U, driver.lastSckPin);
    TEST_ASSERT_EQUAL_UINT8(23U, driver.lastMosiPin);
    TEST_ASSERT_EQUAL_UINT8(kGpioPinUnset, driver.lastMisoPin);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
    TEST_ASSERT_EQUAL_UINT32(1U, bus.generation());

    SpiBusDevice::DependencyTransaction first = bus.beginDependencyTransaction();
    TEST_ASSERT_TRUE(first);
    TEST_ASSERT_TRUE(bus.dependencyTransactionActive());
    TEST_ASSERT_TRUE(first.driver()->beginTransaction(500000U, 0U, 1U));
    TEST_ASSERT_EQUAL_UINT32(1U, driver.beginTransactionCount);
    const uint8_t txBytes[] = {'a', 'b'};
    TEST_ASSERT_EQUAL_UINT32(2U, first.driver()->transferBytes(txBytes, nullptr, 2U));
    first.driver()->endTransaction();

    SpiBusDevice::DependencyTransaction second = bus.beginDependencyTransaction();
    TEST_ASSERT_FALSE(second);
    first.release();
    TEST_ASSERT_FALSE(bus.dependencyTransactionActive());

    FakeSpiDependentRuntime dependentA(12U);
    FakeSpiDependentRuntime dependentB(15U);
    bus.attachDependentRuntime(&dependentA);
    bus.attachDependentRuntime(&dependentB);
    TEST_ASSERT_TRUE(bus.hasDuplicateDependentSpiChipSelect(12U, nullptr));
    TEST_ASSERT_FALSE(bus.hasDuplicateDependentSpiChipSelect(13U, nullptr));
    TEST_ASSERT_FALSE(bus.hasDuplicateDependentSpiChipSelect(12U, &dependentA));
}

void test_spi_runtime_reconfigures_and_advances_generation() {
    FakeSpiBusDriver driver;
    SpiBusDevice bus(makeConfig(), driver);
    driveBusToReady(bus);

    const uint32_t generationBefore = bus.generation();
    SpiBusDevice::DependencyTransaction stale = bus.beginDependencyTransaction();
    TEST_ASSERT_TRUE(stale);
    SpiBusDeviceConfigV2 updated = makeConfig(kSpiBusHostHspi, 25U, 26U, 27U);
    const BoundedBlob<kMaxDeviceConfigBytes> payload = encodePayload(updated);
    const DeviceConfigUpdatePlan plan = bus.planConfigUpdate(payload);
    TEST_ASSERT_TRUE(plan.endOldConfig);
    TEST_ASSERT_TRUE(plan.resetStateMachine);
    TEST_ASSERT_TRUE(bus.applyConfig(payload, 20U));

    bus.requestReconfigure();
    bus.tick100ms(21U);
    bus.tick100ms(22U);

    TEST_ASSERT_TRUE(driver.ended);
    TEST_ASSERT_EQUAL_UINT32(2U, driver.beginCount);
    TEST_ASSERT_EQUAL_UINT32(generationBefore + 1U, bus.generation());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(bus.status()));
    TEST_ASSERT_EQUAL_UINT8(25U, driver.lastSckPin);
    TEST_ASSERT_EQUAL_UINT8(26U, driver.lastMosiPin);
    TEST_ASSERT_EQUAL_UINT8(27U, driver.lastMisoPin);
    TEST_ASSERT_FALSE(stale);
    TEST_ASSERT_NULL(stale.driver());
}

void test_spi_bus_diagnostics_runtime_snapshot_and_reset() {
    FakeSpiBusDriver driver;
    SpiBusDevice bus(makeConfig(), driver);
    driveBusToReady(bus);

    bus.recordDiagnosticsError(7U, 123U);
    TEST_ASSERT_EQUAL_UINT32(1U, bus.diagnostics().consecutiveErrors);
    TEST_ASSERT_EQUAL_UINT32(7U, bus.diagnostics().lastErrorCode);
    TEST_ASSERT_TRUE(bus.handleCommand(DeviceCommand{DeviceCommandType::ResetDiagnostics, 1001U, ""}));
    TEST_ASSERT_EQUAL_UINT32(0U, bus.diagnostics().consecutiveErrors);
    TEST_ASSERT_EQUAL_UINT32(0U, bus.diagnostics().errorOps);

    StaticJsonDocument<512> outputDoc;
    JsonObject output = outputDoc.to<JsonObject>();
    SpiBusDeviceApiAdapter::instance().writeDeviceJson(bus, bus.status(), output);
    TEST_ASSERT_TRUE(output["runtime"]["diagnostics"]["status"].is<const char*>());
}

void test_spi_setup_transfer_round_trip_and_api_serialization() {
    MemoryConfigStorage storage;
    DeviceRegistryStore registryStore(storage);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    FixedDeviceIdSource ids{1001};
    DeviceRegistry registry(registryStore, types, ids);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeCreateRequest(), 0).ok());

    const DeviceApiAdapterRegistry adapters = DeviceApiAdapterRegistry::withDefaults();
    std::string bundle;
    TEST_ASSERT_TRUE(DeviceSetupTransferCodec::writeBundle(bundle, registry, adapters, registry.registryRevision(), "nodemcu-32s"));
    TEST_ASSERT_TRUE(bundle.find("\"typeName\":\"spi_bus\"") != std::string::npos);

    const char* path = "/tmp/spi_bus_device_bundle.ndjson";
    writeTextFile(path, bundle);

    DeviceSetupTransferCodec::ParseResult parsed = DeviceSetupTransferCodec::parseFile(path, bundle.size(), adapters);
    TEST_ASSERT_TRUE(parsed.ok());
    TEST_ASSERT_EQUAL_UINT32(1U, parsed.deviceCount);

    MemoryConfigStorage restoreStorage;
    DeviceRegistryStore restoreStore(restoreStorage);
    DeviceRegistry restored(restoreStore, types, ids);
    TEST_ASSERT_TRUE(restored.restore(parsed.snapshot, parsed.configBlobs, parsed.registryRevision, 0).ok());
    TEST_ASSERT_NOT_NULL(restored.runtime(1001));
    TEST_ASSERT_EQUAL_STRING("spi-bus", restored.runtime(1001)->name());
    TEST_ASSERT_EQUAL_UINT32(SpiBusDevice::descriptor().typeId, restored.runtime(1001)->typeId());
}
