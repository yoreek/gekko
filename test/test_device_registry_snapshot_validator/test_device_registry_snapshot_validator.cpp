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
    record.depCount = 0;
    record.status = DeviceStatus::Ready;
    return record;
}

DeviceRegistrySnapshot makeDependencySnapshot() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({10, 1});
    snapshot.indexEntries.push_back({11, 1});

    DeviceRegistryEntry dependency = makeRecord(10, 1, 2, "bus", "d1");
    DeviceRegistryEntry dependent = makeRecord(11, 1, 2, "sensor", "s1");
    dependent.depCount = 1;
    dependent.deps[0] = {DeviceDependencyRole::OneWireBus, 10};

    snapshot.records.push_back(dependency);
    snapshot.records.push_back(dependent);
    return snapshot;
}

} // namespace

void test_validator_accepts_valid_structure() {
    const DeviceRegistrySnapshot snapshot = makeDependencySnapshot();
    const DeviceValidationResult result = DeviceRegistrySnapshotValidator::validateStructure(snapshot);
    TEST_ASSERT_TRUE(result.ok());
}

void test_validator_rejects_duplicate_device_id() {
    DeviceRegistrySnapshot snapshot = makeDependencySnapshot();
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
    first.depCount = 1;
    first.deps[0] = {DeviceDependencyRole::OneWireBus, 2};
    second.depCount = 1;
    second.deps[0] = {DeviceDependencyRole::OneWireBus, 1};
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

    DeviceRegistryEntry dependency = makeRecord(100, 10, 1, "dependency", "d");
    DeviceRegistryEntry dependentA = makeRecord(101, 11, 1, "dependent-a", "a");
    DeviceRegistryEntry dependentB = makeRecord(102, 11, 1, "dependent-b", "b");
    dependentA.depCount = 1;
    dependentA.deps[0] = {DeviceDependencyRole::OneWireBus, 100};
    dependentB.depCount = 1;
    dependentB.deps[0] = {DeviceDependencyRole::OneWireBus, 100};
    snapshot.records.push_back(dependency);
    snapshot.records.push_back(dependentA);
    snapshot.records.push_back(dependentB);

    DeviceTypeRegistry types{};
    DeviceTypeDescriptor dependencyDescriptor{};
    dependencyDescriptor.typeId = 10;
    dependencyDescriptor.name = "Dependency";
    dependencyDescriptor.currentConfigVersion = 1;
    dependencyDescriptor.maxDependents = 1;
    TEST_ASSERT_TRUE(types.registerDescriptor(dependencyDescriptor));

    DeviceTypeDescriptor dependentDescriptor{};
    dependentDescriptor.typeId = 11;
    dependentDescriptor.name = "Dependent";
    dependentDescriptor.currentConfigVersion = 1;
    dependentDescriptor.dependencyRequirements.push_back({DeviceDependencyRole::OneWireBus, true, {10}});
    TEST_ASSERT_TRUE(types.registerDescriptor(dependentDescriptor));

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
