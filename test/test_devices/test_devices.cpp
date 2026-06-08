#include "config/MemoryConfigStorage.h"
#include "devices/DeviceIdGenerator.h"
#include "devices/DeviceRegistryStore.h"
#include "devices/DummyDevice.h"
#include "devices/RetainedStateStore.h"

#include <type_traits>
#include <unity.h>

using namespace ewfm;

namespace {

template <typename T>
void appendLE(std::string& out, T value) {
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

DeviceRecord makeDummyRecord(DeviceId id, DeviceId parentId, bool hasParent, const std::string& name, const DummyDeviceConfigV2& config) {
    DeviceRecord record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = id;
    record.header.typeId = 1;
    record.header.configVersion = 2;
    record.header.configRevision = 7;
    record.header.payloadLength = static_cast<uint32_t>(encodeDummyConfig(config).size());
    record.name = name;
    record.enabled = true;
    record.hasParent = hasParent;
    record.parentDeviceId = parentId;
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    record.status = DeviceStatus::Ready;
    record.configPayload = encodeDummyConfig(config);
    return record;
}

} // namespace

void test_default_device_type_registry_contains_dummy() {
    DeviceTypeRegistry registry = DeviceTypeRegistry::withDefaults();
    const DeviceTypeDescriptor* descriptor = registry.find(1);
    TEST_ASSERT_NOT_NULL(descriptor);
    TEST_ASSERT_EQUAL_STRING("DummyDevice", descriptor->name);
    TEST_ASSERT_EQUAL_UINT32(2, descriptor->currentConfigVersion);
    TEST_ASSERT_TRUE(descriptor->supportsCommands);
    TEST_ASSERT_TRUE(descriptor->supportsRetainedState);
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

    DummyDeviceConfigV2 config{};
    config.restorePreviousState = true;
    config.defaultOutput = false;
    config.currentOutput = true;
    config.inverted = false;

    DeviceRegistrySnapshot snapshot;
    snapshot.indexEntries.push_back({1, 1});
    snapshot.indexEntries.push_back({2, 1});
    snapshot.records.push_back(makeDummyRecord(1, 0, false, "bus", config));

    DummyDeviceConfigV2 childConfig = config;
    childConfig.currentOutput = false;
    snapshot.records.push_back(makeDummyRecord(2, 1, true, "sensor", childConfig));

    DeviceValidationResult saveResult = store.save(snapshot);
    TEST_ASSERT_TRUE(saveResult.ok());

    DeviceRegistrySnapshot loaded;
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceValidationResult loadResult = store.load(loaded, &types);
    TEST_ASSERT_TRUE(loadResult.ok());
    TEST_ASSERT_EQUAL_UINT32(2, loaded.records.size());
    TEST_ASSERT_EQUAL_UINT32(2, loaded.indexEntries.size());
    TEST_ASSERT_EQUAL_UINT32(1, loaded.records[0].header.deviceId);
    TEST_ASSERT_EQUAL_UINT32(2, loaded.records[1].header.deviceId);
    TEST_ASSERT_EQUAL_STRING("bus", loaded.records[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("sensor", loaded.records[1].name.c_str());
    TEST_ASSERT_TRUE(loaded.records[1].hasParent);
    TEST_ASSERT_EQUAL_UINT32(1, loaded.records[1].parentDeviceId);
    TEST_ASSERT_EQUAL_STRING(snapshot.records[1].configPayload.c_str(), loaded.records[1].configPayload.c_str());
}

void test_device_registry_store_rejects_corrupt_index() {
    MemoryConfigStorage storage;
    TEST_ASSERT_TRUE(storage.putString("index", std::string("\x00\x01", 2)));

    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(true));

    DeviceRegistrySnapshot loaded;
    DeviceValidationResult result = store.load(loaded, nullptr);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::CorruptRecord), static_cast<int>(result.error));
}

void test_retained_state_store_round_trip_and_remove() {
    MemoryConfigStorage storage;
    RetainedStateStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    RetainedStateRecord record{};
    record.deviceId = 7;
    record.payload = "last-output=1";
    TEST_ASSERT_TRUE(store.save(record).ok());

    RetainedStateRecord loaded;
    DeviceValidationResult loadResult = store.load(7, loaded);
    TEST_ASSERT_TRUE(loadResult.ok());
    TEST_ASSERT_EQUAL_UINT32(7, loaded.deviceId);
    TEST_ASSERT_EQUAL_STRING(record.payload.c_str(), loaded.payload.c_str());

    TEST_ASSERT_TRUE(store.remove(7));
    RetainedStateRecord missing;
    DeviceValidationResult missingResult = store.load(7, missing);
    TEST_ASSERT_FALSE(missingResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::MissingRecord), static_cast<int>(missingResult.error));
}

void test_retained_state_store_rejects_corrupt_payload() {
    MemoryConfigStorage storage;
    TEST_ASSERT_TRUE(storage.putString("state_00000007", std::string("bad", 3)));

    RetainedStateStore store(storage);
    TEST_ASSERT_TRUE(store.begin(true));

    RetainedStateRecord loaded;
    DeviceValidationResult result = store.load(7, loaded);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::CorruptRecord), static_cast<int>(result.error));
}

void test_dummy_device_lifecycle_and_retained_restore() {
    DummyDeviceConfigV2 config{};
    config.enabled = true;
    config.restorePreviousState = true;
    config.defaultOutput = false;
    config.currentOutput = false;

    DeviceRecord record = makeDummyRecord(3, 0, false, "dummy", config);
    DummyDevice device(record);

    device.applyRetainedState(true);
    device.begin(100);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Creating), static_cast<int>(device.status()));

    device.tickFastLoop(101);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.outputState());

    TEST_ASSERT_TRUE(device.handleCommand(DeviceCommand{DeviceCommandType::Custom, 3, "output=0", DevicePersistencePolicy::Delayed}));
    TEST_ASSERT_FALSE(device.outputState());

    device.requestDisable();
    device.tick100ms(102);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(device.status()));

    device.requestDelete();
    device.tick1s(103);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Deleting), static_cast<int>(device.status()));
    TEST_ASSERT_TRUE(device.deleted());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_default_device_type_registry_contains_dummy);
    RUN_TEST(test_device_id_generation_skips_reserved_and_duplicates);
    RUN_TEST(test_device_id_generation_exhaustion_fails);
    RUN_TEST(test_device_registry_store_round_trip);
    RUN_TEST(test_device_registry_store_rejects_corrupt_index);
    RUN_TEST(test_retained_state_store_round_trip_and_remove);
    RUN_TEST(test_retained_state_store_rejects_corrupt_payload);
    RUN_TEST(test_dummy_device_lifecycle_and_retained_restore);
    return UNITY_END();
}
