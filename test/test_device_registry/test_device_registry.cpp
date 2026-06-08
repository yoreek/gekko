#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"

#include <type_traits>
#include <unity.h>

using namespace ewfm;

namespace {

template <typename T> void appendLE(std::string& out, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    const Unsigned v = static_cast<Unsigned>(value);
    for (size_t index = 0; index < sizeof(T); ++index) {
        out.push_back(static_cast<char>((v >> (index * 8)) & 0xFFU));
    }
}

std::string encodeDummyConfig(const DummyDeviceConfigV2& config) {
    std::string blob;
    blob.reserve(16);
    appendLE<uint32_t>(blob, DummyDeviceConfigV2::magicKey);
    appendLE<uint8_t>(blob, config.enabled ? 1U : 0U);
    appendLE<uint8_t>(blob, config.restorePreviousState ? 1U : 0U);
    appendLE<uint8_t>(blob, config.defaultOutput ? 1U : 0U);
    appendLE<uint8_t>(blob, config.currentOutput ? 1U : 0U);
    appendLE<uint8_t>(blob, config.inverted ? 1U : 0U);
    appendLE<uint8_t>(blob, 0U);
    appendLE<uint8_t>(blob, 0U);
    appendLE<uint8_t>(blob, 0U);
    return blob;
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

DeviceCreateRequest makeDummyCreateRequest(const std::string& name, bool enabled = true) {
    DummyDeviceConfigV2 config{};
    config.enabled = enabled;
    config.restorePreviousState = true;
    config.defaultOutput = false;
    config.currentOutput = false;
    config.inverted = false;

    DeviceCreateRequest request{};
    request.typeId = 1;
    request.name = name;
    request.enabled = enabled;
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.configPayload = encodeDummyConfig(config);
    request.persistencePolicy = DevicePersistencePolicy::Immediate;
    return request;
}

} // namespace

void test_registry_begin_loads_runtime_devices() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DeviceRegistrySnapshot snapshot;
    DeviceRecord record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = 11;
    record.header.typeId = 1;
    record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    record.header.payloadLength = static_cast<uint32_t>(encodeDummyConfig(DummyDeviceConfigV2{}).size());
    record.name = "dummy";
    record.enabled = true;
    record.status = DeviceStatus::Ready;
    record.configPayload = encodeDummyConfig(DummyDeviceConfigV2{});
    snapshot.indexEntries.push_back({11, 1});
    snapshot.records.push_back(record);
    TEST_ASSERT_TRUE(store.save(snapshot).ok());

    FixedDeviceIdSource idSource({21, 22});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);

    TEST_ASSERT_TRUE(registry.begin(100).ok());
    TEST_ASSERT_EQUAL_UINT32(1, registry.list().size());
    TEST_ASSERT_NOT_NULL(registry.find(11));
    TEST_ASSERT_NOT_NULL(registry.runtime(11));
    TEST_ASSERT_EQUAL_UINT32(0, registry.registryRevision());
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());
}

void test_registry_create_rename_and_flush() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({101, 102});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult createResult = registry.create(makeDummyCreateRequest("dummy-a"), 10);
    TEST_ASSERT_TRUE(createResult.ok());
    TEST_ASSERT_EQUAL_UINT32(101, createResult.deviceId);
    TEST_ASSERT_FALSE(createResult.pendingPersistence);
    TEST_ASSERT_NOT_NULL(registry.find(101));
    TEST_ASSERT_NOT_NULL(registry.runtime(101));

    DeviceMutationResult renameResult = registry.rename(101, "dummy-b", 20, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE(renameResult.ok());
    TEST_ASSERT_TRUE(renameResult.pendingPersistence);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
    TEST_ASSERT_EQUAL_STRING("dummy-b", registry.find(101)->name.c_str());

    TEST_ASSERT_TRUE(registry.flushNow().ok());
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());

    DeviceRegistrySnapshot loaded;
    TEST_ASSERT_TRUE(store.load(loaded, &types).ok());
    TEST_ASSERT_EQUAL_UINT32(1, loaded.records.size());
    TEST_ASSERT_EQUAL_STRING("dummy-b", loaded.records[0].name.c_str());
}

void test_registry_disable_enable_and_delete() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({201, 202});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("dummy"), 10).ok());
    TEST_ASSERT_NOT_NULL(registry.runtime(201));

    DeviceMutationResult disableResult = registry.setEnabled(201, false, 20, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(disableResult.ok());
    TEST_ASSERT_NULL(registry.runtime(201));

    DeviceMutationResult enableResult = registry.setEnabled(201, true, 30, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(enableResult.ok());
    TEST_ASSERT_NOT_NULL(registry.runtime(201));

    DeviceMutationResult deleteResult = registry.remove(201, 40, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(deleteResult.ok());
    TEST_ASSERT_NULL(registry.find(201));
    TEST_ASSERT_NULL(registry.runtime(201));
}

void test_registry_rejects_duplicate_name_and_unsupported_type() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({301, 302});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("same"), 10).ok());

    DeviceCreateRequest duplicate = makeDummyCreateRequest("same");
    DeviceCreateResult duplicateResult = registry.create(duplicate, 20);
    TEST_ASSERT_FALSE(duplicateResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidConfig), static_cast<int>(duplicateResult.validation.error));

    DeviceCreateRequest unsupported = makeDummyCreateRequest("other");
    unsupported.typeId = 99;
    DeviceCreateResult unsupportedResult = registry.create(unsupported, 30);
    TEST_ASSERT_FALSE(unsupportedResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::UnsupportedType), static_cast<int>(unsupportedResult.validation.error));
}

void test_registry_delayed_flushes_on_tick() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({401, 402});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("tick-dummy"), 10).ok());

    DeviceMutationResult renameResult = registry.rename(401, "tick-dummy-2", 20, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE(renameResult.ok());
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());

    registry.tick(400);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());

    registry.tick(600);
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());

    DeviceRegistrySnapshot loaded;
    TEST_ASSERT_TRUE(store.load(loaded, &types).ok());
    TEST_ASSERT_EQUAL_STRING("tick-dummy-2", loaded.records[0].name.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_begin_loads_runtime_devices);
    RUN_TEST(test_registry_create_rename_and_flush);
    RUN_TEST(test_registry_disable_enable_and_delete);
    RUN_TEST(test_registry_rejects_duplicate_name_and_unsupported_type);
    RUN_TEST(test_registry_delayed_flushes_on_tick);
    return UNITY_END();
}
