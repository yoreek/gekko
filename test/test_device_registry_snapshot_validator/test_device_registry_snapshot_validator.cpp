#include "devices/registry/DeviceRegistryBinaryCodec.h"
#include "devices/registry/DeviceRegistrySnapshotValidator.h"

#include <unity.h>

using namespace ewfm;

namespace {

DeviceRegistryEntry makeRecord(DeviceId id, DeviceTypeId typeId, uint32_t configVersion, const char* name, const std::string& payload) {
    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = id;
    record.header.typeId = typeId;
    record.header.configVersion = configVersion;
    record.header.configRevision = 1;
    record.header.payloadLength = static_cast<uint32_t>(payload.size());
    (void)name;
    record.hasParent = false;
    record.parentDeviceId = 0;
    record.status = DeviceStatus::Ready;
    record.persistencePolicy = DevicePersistencePolicy::Delayed;
    return record;
}

DeviceRegistrySnapshot makeParentChildSnapshot() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({10, 1});
    snapshot.indexEntries.push_back({11, 1});

    DeviceRegistryEntry parent = makeRecord(10, 1, 2, "bus", "p1");
    DeviceRegistryEntry child = makeRecord(11, 1, 2, "sensor", "c1");
    child.hasParent = true;
    child.parentDeviceId = 10;

    snapshot.records.push_back(parent);
    snapshot.records.push_back(child);
    return snapshot;
}

} // namespace

void test_validator_accepts_valid_structure() {
    const DeviceRegistrySnapshot snapshot = makeParentChildSnapshot();
    const DeviceValidationResult result = DeviceRegistrySnapshotValidator::validateStructure(snapshot);
    TEST_ASSERT_TRUE(result.ok());
}

void test_validator_rejects_duplicate_device_id() {
    DeviceRegistrySnapshot snapshot = makeParentChildSnapshot();
    snapshot.indexEntries[1].deviceId = snapshot.indexEntries[0].deviceId;

    const DeviceValidationResult result = DeviceRegistrySnapshotValidator::validateStructure(snapshot);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::DuplicateDeviceId), static_cast<int>(result.error));
}

void test_validator_rejects_cycle() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({1, 1});
    snapshot.indexEntries.push_back({2, 1});

    DeviceRegistryEntry first = makeRecord(1, 1, 2, "first", "a");
    DeviceRegistryEntry second = makeRecord(2, 1, 2, "second", "b");
    first.hasParent = true;
    first.parentDeviceId = 2;
    second.hasParent = true;
    second.parentDeviceId = 1;
    snapshot.records.push_back(first);
    snapshot.records.push_back(second);

    const DeviceValidationResult result = DeviceRegistrySnapshotValidator::validateStructure(snapshot);
    TEST_ASSERT_FALSE(result.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(result.error));
}

void test_validator_typed_relationship_checks() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({100, 10});
    snapshot.indexEntries.push_back({101, 11});
    snapshot.indexEntries.push_back({102, 11});

    DeviceRegistryEntry parent = makeRecord(100, 10, 1, "parent", "p");
    DeviceRegistryEntry childA = makeRecord(101, 11, 1, "child-a", "a");
    DeviceRegistryEntry childB = makeRecord(102, 11, 1, "child-b", "b");
    childA.hasParent = true;
    childA.parentDeviceId = 100;
    childB.hasParent = true;
    childB.parentDeviceId = 100;
    snapshot.records.push_back(parent);
    snapshot.records.push_back(childA);
    snapshot.records.push_back(childB);

    DeviceTypeRegistry types{};
    DeviceTypeDescriptor parentDescriptor{};
    parentDescriptor.typeId = 10;
    parentDescriptor.name = "Parent";
    parentDescriptor.currentConfigVersion = 1;
    parentDescriptor.canHaveChildren = true;
    parentDescriptor.maxChildren = 1;
    TEST_ASSERT_TRUE(types.registerDescriptor(parentDescriptor));

    DeviceTypeDescriptor childDescriptor{};
    childDescriptor.typeId = 11;
    childDescriptor.name = "Child";
    childDescriptor.currentConfigVersion = 1;
    childDescriptor.compatibleParentTypes.push_back(10);
    TEST_ASSERT_TRUE(types.registerDescriptor(childDescriptor));

    const DeviceValidationResult structure = DeviceRegistrySnapshotValidator::validateStructure(snapshot);
    TEST_ASSERT_TRUE(structure.ok());

    const DeviceValidationResult typed = DeviceRegistrySnapshotValidator::validateTypedRelationships(snapshot, &types);
    TEST_ASSERT_FALSE(typed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(typed.error));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_validator_accepts_valid_structure);
    RUN_TEST(test_validator_rejects_duplicate_device_id);
    RUN_TEST(test_validator_rejects_cycle);
    RUN_TEST(test_validator_typed_relationship_checks);
    return UNITY_END();
}
