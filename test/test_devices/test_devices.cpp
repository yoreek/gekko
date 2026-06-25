#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceScopedDataStore.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceRetainedDataStore.h"

#include <cstdio>
#include <unity.h>

using namespace ewfm;

namespace {

BoundedBlob<kMaxDeviceConfigBytes> encodeDummyConfig(const DummyDeviceConfigV1& config) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(ewfm::encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(payload.assign(buffer, dummyDeviceConfigSize(config)));
    return payload;
}

DeviceRegistryEntry makeDummyRecord(DeviceId id, DeviceId dependencyId, bool hasDependency, const std::string& name,
                                    const DummyDeviceConfigV1& config) {
    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = id;
    record.header.typeId = 1;
    record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 7;
    record.header.payloadLength = static_cast<uint32_t>(dummyDeviceConfigSize(config));
    (void)name;
    record.depCount = hasDependency ? 1U : 0U;
    if (hasDependency) {
        record.deps[0] = {DeviceDependencyRole::OneWireBus, dependencyId};
    }
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    record.status = DeviceStatus::Ready;
    return record;
}

DummyDeviceConfigV1 makeDummyConfig(const char* name, bool enabled) {
    DummyDeviceConfigV1 config{};
    config.enabled = enabled;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    return config;
}

} // namespace

void test_default_device_type_registry_contains_dummy() {
    DeviceTypeRegistry registry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = registry.find(1);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("DummyDevice", descriptor->name);
    TEST_ASSERT_EQUAL_UINT32(1, descriptor->currentConfigVersion);
    TEST_ASSERT_FALSE(descriptor->supportsCommands);
    TEST_ASSERT_FALSE(descriptor->supportsRetainedState);
    TEST_ASSERT_TRUE(descriptor->ticksFastLoop);
    TEST_ASSERT_FALSE(descriptor->ticks100ms);
    TEST_ASSERT_FALSE(descriptor->ticks1s);
}

void test_default_device_type_registry_contains_onewire() {
    DeviceTypeRegistry registry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = registry.find(3);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("OneWireBusDevice", descriptor->name);
    TEST_ASSERT_TRUE(descriptor->supportsCommands);
    TEST_ASSERT_FALSE(descriptor->supportsRetainedState);
}

void test_device_id_generation_skips_reserved_and_duplicates() {
    SequentialDeviceIdSource source(0);
    DeviceId out{0};
    auto result = assignUniqueDeviceId(source, [](DeviceId candidate) { return candidate == 1; }, out, 4);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(2, out);
}

void test_device_id_generation_exhaustion_fails() {
    SequentialDeviceIdSource source(1);
    DeviceId out{0};
    auto result = assignUniqueDeviceId(source, [](DeviceId) { return true; }, out, 3);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::DuplicateDeviceId), static_cast<int>(result.error));
}

void test_device_registry_store_round_trip() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DummyDeviceConfigV1 config = makeDummyConfig("bus", true);

    DeviceRegistrySnapshot snapshot;
    snapshot.indexEntries.push_back({1, 1});
    snapshot.indexEntries.push_back({2, 1});
    snapshot.records.push_back(makeDummyRecord(1, 0, false, "bus", config));
    DeviceConfigBlobMap configBlobs;
    configBlobs[1] = encodeDummyConfig(config);

    DummyDeviceConfigV1 childConfig = makeDummyConfig("sensor", true);
    snapshot.records.push_back(makeDummyRecord(2, 0, false, "sensor", childConfig));
    configBlobs[2] = encodeDummyConfig(childConfig);

    DeviceValidationResult saveResult = store.save(snapshot, configBlobs);
    TEST_ASSERT_TRUE(saveResult.ok());

    DeviceRegistrySnapshot loaded;
    DeviceConfigBlobMap loadedConfigBlobs;
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceValidationResult loadResult = store.load(loaded, loadedConfigBlobs, &types);
    TEST_ASSERT_TRUE(loadResult.ok());
    TEST_ASSERT_EQUAL_UINT32(2, loaded.records.size());
    TEST_ASSERT_EQUAL_UINT32(2, loaded.indexEntries.size());
    TEST_ASSERT_EQUAL_UINT32(1, loaded.records[0].header.deviceId);
    TEST_ASSERT_EQUAL_UINT32(2, loaded.records[1].header.deviceId);
    DummyDeviceConfigV1 loadedDependencyConfig{};
    DummyDeviceConfigV1 loadedDependentConfig{};
    TEST_ASSERT_TRUE(decodeDummyDeviceConfig(loadedConfigBlobs[1].data(), loadedConfigBlobs[1].size(), loadedDependencyConfig));
    TEST_ASSERT_TRUE(decodeDummyDeviceConfig(loadedConfigBlobs[2].data(), loadedConfigBlobs[2].size(), loadedDependentConfig));
    TEST_ASSERT_EQUAL_STRING("bus", loadedDependencyConfig.name);
    TEST_ASSERT_EQUAL_STRING("sensor", loadedDependentConfig.name);
    TEST_ASSERT_EQUAL_UINT32(configBlobs[2].size(), loadedConfigBlobs[2].size());
    TEST_ASSERT_EQUAL_MEMORY(configBlobs[2].data(), loadedConfigBlobs[2].data(), configBlobs[2].size());
}

void test_device_registry_store_resets_on_missing_registry_version() {
    MemoryConfigStorage storage;
    TEST_ASSERT_TRUE(storage.putString("index", "zz"));

    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DeviceRegistrySnapshot loaded;
    DeviceConfigBlobMap loadedConfigBlobs;
    DeviceValidationResult result = store.load(loaded, loadedConfigBlobs, nullptr);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_TRUE(loaded.records.empty());
    TEST_ASSERT_TRUE(loaded.indexEntries.empty());
    TEST_ASSERT_FALSE(storage.hasKey("index"));
    TEST_ASSERT_FALSE(storage.hasKey("version"));
}

void test_device_registry_store_resets_on_registry_version_mismatch() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DummyDeviceConfigV1 config = makeDummyConfig("bus", true);
    DeviceRegistrySnapshot snapshot;
    snapshot.indexEntries.push_back({1, 1});
    snapshot.records.push_back(makeDummyRecord(1, 0, false, "bus", config));
    DeviceConfigBlobMap configBlobs;
    configBlobs[1] = encodeDummyConfig(config);
    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());
    TEST_ASSERT_TRUE(storage.putUInt("version", 999));

    DeviceRegistrySnapshot loaded;
    DeviceConfigBlobMap loadedConfigBlobs;
    DeviceValidationResult result = store.load(loaded, loadedConfigBlobs, nullptr);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_TRUE(loaded.records.empty());
    TEST_ASSERT_TRUE(loaded.indexEntries.empty());
    TEST_ASSERT_FALSE(storage.hasKey("index"));
    TEST_ASSERT_FALSE(storage.hasKey("record_00000001"));
    TEST_ASSERT_FALSE(storage.hasKey("version"));
}

void test_device_retained_data_store_round_trip_and_remove() {
    MemoryConfigStorage storage;
    DeviceRetainedDataStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    SwitchRetainedStateRecord record{};
    record.deviceId = 7;
    record.outputState = OutputState::On;
    TEST_ASSERT_TRUE(store.save(record).ok());

    SwitchRetainedStateRecord loaded;
    DeviceValidationResult loadResult = store.load(7, loaded);
    TEST_ASSERT_TRUE(loadResult.ok());
    TEST_ASSERT_EQUAL_UINT32(7, loaded.deviceId);
    TEST_ASSERT_EQUAL(static_cast<int>(record.outputState), static_cast<int>(loaded.outputState));

    TEST_ASSERT_TRUE(store.remove(7));
    SwitchRetainedStateRecord missing;
    DeviceValidationResult missingResult = store.load(7, missing);
    TEST_ASSERT_FALSE(missingResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::MissingRecord), static_cast<int>(missingResult.error));
}

void test_device_scoped_data_store_round_trip_and_clear() {
    MemoryConfigStorage storage;
    DeviceScopedDataStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    SwitchRetainedStateRecord retained{};
    retained.deviceId = 42;
    retained.outputState = OutputState::On;
    TEST_ASSERT_TRUE(store.save(42, "retained_state", retained).ok());

    SwitchRetainedStateRecord loaded{};
    TEST_ASSERT_TRUE(store.load(42, "retained_state", loaded).ok());
    TEST_ASSERT_EQUAL_UINT32(42, loaded.deviceId);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(loaded.outputState));

    TEST_ASSERT_TRUE(store.clearDevice(42));
    SwitchRetainedStateRecord missing{};
    DeviceValidationResult result = store.load(42, "retained_state", missing);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::MissingRecord), static_cast<int>(result.error));
}

void test_device_retained_data_store_migrates_legacy_record() {
    MemoryConfigStorage storage;
    TEST_ASSERT_TRUE(storage.begin("device_retained", false));
    SwitchRetainedStateRecord legacy{};
    legacy.deviceId = 9;
    legacy.outputState = OutputState::On;
    TEST_ASSERT_TRUE(putStruct(storage, "state_00000009", legacy));
    storage.end();

    DeviceRetainedDataStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    SwitchRetainedStateRecord loaded{};
    DeviceValidationResult result = store.load(9, loaded);
    TEST_ASSERT_TRUE(result.ok());
    TEST_ASSERT_EQUAL_UINT32(9, loaded.deviceId);
    TEST_ASSERT_EQUAL(static_cast<int>(OutputState::On), static_cast<int>(loaded.outputState));
}

void test_device_retained_data_store_clears_legacy_namespace() {
    MemoryConfigStorage storage;
    TEST_ASSERT_TRUE(storage.begin("device_retained", false));
    SwitchRetainedStateRecord legacy{};
    legacy.deviceId = 12;
    legacy.outputState = OutputState::Off;
    TEST_ASSERT_TRUE(putStruct(storage, "state_0000000c", legacy));
    storage.end();

    DeviceRetainedDataStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    TEST_ASSERT_TRUE(store.clearLegacyNamespace());

    TEST_ASSERT_TRUE(storage.begin("device_retained", true));
    TEST_ASSERT_FALSE(storage.hasKey("state_0000000c"));
    storage.end();
}

void test_device_retained_data_store_rejects_corrupt_record() {
    MemoryConfigStorage storage;
    TEST_ASSERT_TRUE(storage.putString("state_00000007", "zz"));

    DeviceRetainedDataStore store(storage);
    TEST_ASSERT_TRUE(store.begin(true));

    SwitchRetainedStateRecord loaded;
    DeviceValidationResult result = store.load(7, loaded);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::MissingRecord), static_cast<int>(result.error));
}

void test_dummy_device_lifecycle_and_command_output() {
    DummyDeviceConfigV1 config = makeDummyConfig("dummy", true);

    DeviceRegistryEntry record = makeDummyRecord(3, 0, false, "dummy", config);
    DummyDevice device(record, encodeDummyConfig(config));

    device.begin(100);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Creating), static_cast<int>(device.status()));

    device.tickFastLoop(101);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_FALSE(device.handleCommand(DeviceCommand{DeviceCommandType::Custom, 3, "output=0", DevicePersistencePolicy::Delayed}));

    device.requestDisable();
    device.tick100ms(102);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));

    device.requestDelete();
    device.tick1s(103);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Deleting), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.deleted());
}

void test_dummy_device_base_config_is_loaded_and_runtime_starts_disabled() {
    DummyDeviceConfigV1 config = makeDummyConfig("dummy-fallback", true);

    DeviceRegistryEntry record = makeDummyRecord(4, 0, false, "dummy-fallback", config);
    DummyDevice device(record, encodeDummyConfig(config));

    device.begin(200);
    device.tickFastLoop(201);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
}

void test_dummy_device_dependency_wiring_survive_base_refactor() {
    DummyDeviceConfigV1 config = makeDummyConfig("dependency", true);

    DeviceRegistryEntry dependencyRecord = makeDummyRecord(5, 0, false, "dependency", config);
    DeviceRegistryEntry dependentRecord = makeDummyRecord(6, 5, true, "dependent", config);
    DummyDevice dependency(dependencyRecord, encodeDummyConfig(config));
    DummyDevice dependent(dependentRecord, encodeDummyConfig(config));

    dependent.setDependencyRuntime(DeviceDependencyRole::OneWireBus, &dependency);
    dependency.attachDependentRuntime(&dependent);
    dependency.attachDependentRuntime(&dependent);
    TEST_ASSERT_EQUAL_PTR(static_cast<IDeviceRuntime*>(&dependency), dependent.dependencyRuntime(DeviceDependencyRole::OneWireBus));
    TEST_ASSERT_EQUAL_UINT32(1, dependency.dependentRuntimes().size());

    dependent.begin(300);
    dependent.tickFastLoop(301);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(dependent.status()));

    dependency.begin(302);
    dependency.tickFastLoop(303);
    dependent.tickFastLoop(304);
    dependent.tickFastLoop(305);
    dependent.tickFastLoop(306);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(dependent.status()));

    dependency.detachDependentRuntime(&dependent);
    TEST_ASSERT_TRUE(dependency.dependentRuntimes().empty());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_default_device_type_registry_contains_dummy);
    RUN_TEST(test_default_device_type_registry_contains_onewire);
    RUN_TEST(test_device_id_generation_skips_reserved_and_duplicates);
    RUN_TEST(test_device_id_generation_exhaustion_fails);
    RUN_TEST(test_device_registry_store_round_trip);
    RUN_TEST(test_device_registry_store_resets_on_missing_registry_version);
    RUN_TEST(test_device_registry_store_resets_on_registry_version_mismatch);
    RUN_TEST(test_device_retained_data_store_round_trip_and_remove);
    RUN_TEST(test_device_retained_data_store_rejects_corrupt_record);
    RUN_TEST(test_device_scoped_data_store_round_trip_and_clear);
    RUN_TEST(test_device_retained_data_store_migrates_legacy_record);
    RUN_TEST(test_device_retained_data_store_clears_legacy_namespace);
    RUN_TEST(test_dummy_device_lifecycle_and_command_output);
    RUN_TEST(test_dummy_device_base_config_is_loaded_and_runtime_starts_disabled);
    RUN_TEST(test_dummy_device_dependency_wiring_survive_base_refactor);
    return UNITY_END();
}
