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
    dependent.deps[0] = {DeviceRole::OneWireBus, 10};

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
    first.deps[0] = {DeviceRole::OneWireBus, 2};
    second.depCount = 1;
    second.deps[0] = {DeviceRole::OneWireBus, 1};
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
    dependentA.deps[0] = {DeviceRole::OneWireBus, 100};
    dependentB.depCount = 1;
    dependentB.deps[0] = {DeviceRole::OneWireBus, 100};
    snapshot.records.push_back(dependency);
    snapshot.records.push_back(dependentA);
    snapshot.records.push_back(dependentB);

    DeviceTypeRegistry types{};
    DeviceTypeDescriptor dependencyDescriptor{};
    dependencyDescriptor.typeId = 10;
    dependencyDescriptor.name = "Dependency";
    dependencyDescriptor.currentConfigVersion = 1;
    dependencyDescriptor.maxDependents = 1;
    dependencyDescriptor.providedRoles = ProvidedRoles::of({DeviceRole::OneWireBus});
    TEST_ASSERT_TRUE(types.registerDescriptor(dependencyDescriptor));

    DeviceTypeDescriptor dependentDescriptor{};
    dependentDescriptor.typeId = 11;
    dependentDescriptor.name = "Dependent";
    dependentDescriptor.currentConfigVersion = 1;
    dependentDescriptor.dependencyRequirements.push_back({DeviceRole::OneWireBus, true});
    TEST_ASSERT_TRUE(types.registerDescriptor(dependentDescriptor));

    const DeviceValidationResult structure = DeviceRegistrySnapshotValidator::validateStructure(snapshot);
    TEST_ASSERT_TRUE(structure.ok());

    const DeviceValidationResult typed = DeviceRegistrySnapshotValidator::validateTypedRelationships(snapshot, &types);
    TEST_ASSERT_FALSE(typed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(typed.error));
}

void test_validator_rejects_dependency_whose_provided_role_does_not_match_requirement() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({100, 10});
    snapshot.indexEntries.push_back({101, 11});

    DeviceRegistryEntry dependency = makeRecord(100, 10, 1, "dependency", "d");
    DeviceRegistryEntry dependent = makeRecord(101, 11, 1, "dependent", "a");
    dependent.depCount = 1;
    dependent.deps[0] = {DeviceRole::OneWireBus, 100};
    snapshot.records.push_back(dependency);
    snapshot.records.push_back(dependent);

    DeviceTypeRegistry types{};
    DeviceTypeDescriptor dependencyDescriptor{};
    dependencyDescriptor.typeId = 10;
    dependencyDescriptor.name = "Dependency";
    dependencyDescriptor.currentConfigVersion = 1;
    // Deliberately does NOT provide the OneWireBus role the dependent requires (defaults to
    // DeviceRole::Unknown) -- this is what the "incompatible dependency type" rejection guards.
    TEST_ASSERT_TRUE(types.registerDescriptor(dependencyDescriptor));

    DeviceTypeDescriptor dependentDescriptor{};
    dependentDescriptor.typeId = 11;
    dependentDescriptor.name = "Dependent";
    dependentDescriptor.currentConfigVersion = 1;
    dependentDescriptor.dependencyRequirements.push_back({DeviceRole::OneWireBus, true});
    TEST_ASSERT_TRUE(types.registerDescriptor(dependentDescriptor));

    const DeviceValidationResult typed = DeviceRegistrySnapshotValidator::validateTypedRelationships(snapshot, &types);
    TEST_ASSERT_FALSE(typed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(typed.error));
}

// A Condition-role dependency link is satisfied by a device whose providedRoles set includes
// Condition alongside its primary role (e.g. a switch also usable as an AND-condition source) --
// the same "list contains" check that ordinary single-role requirements use, not a special case.
void test_validator_accepts_condition_role_dependency_via_secondary_provided_role() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({200, 20});
    snapshot.indexEntries.push_back({201, 21});

    DeviceRegistryEntry dependency = makeRecord(200, 20, 1, "switch", "d");
    DeviceRegistryEntry dependent = makeRecord(201, 21, 1, "auto-switch", "a");
    dependent.depCount = 1;
    dependent.deps[0] = DeviceDependencyLink{DeviceRole::Condition, 200, false};
    snapshot.records.push_back(dependency);
    snapshot.records.push_back(dependent);

    DeviceTypeRegistry types{};
    DeviceTypeDescriptor dependencyDescriptor{};
    dependencyDescriptor.typeId = 20;
    dependencyDescriptor.name = "Switch";
    dependencyDescriptor.currentConfigVersion = 1;
    // Its primary role is Switch, but it also provides Condition -- satisfies both a Switch-role
    // requirement elsewhere and a Condition-role requirement here.
    dependencyDescriptor.providedRoles = ProvidedRoles::of({DeviceRole::Switch, DeviceRole::Condition});
    TEST_ASSERT_TRUE(types.registerDescriptor(dependencyDescriptor));

    DeviceTypeDescriptor dependentDescriptor{};
    dependentDescriptor.typeId = 21;
    dependentDescriptor.name = "AutoSwitchLike";
    dependentDescriptor.currentConfigVersion = 1;
    dependentDescriptor.dependencyRequirements.push_back({DeviceRole::Condition, false});
    TEST_ASSERT_TRUE(types.registerDescriptor(dependentDescriptor));

    const DeviceValidationResult typed = DeviceRegistrySnapshotValidator::validateTypedRelationships(snapshot, &types);
    TEST_ASSERT_TRUE(typed.ok());
}

void test_validator_rejects_second_exclusive_analog_output_owner() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.indexEntries.push_back({300, 1});
    snapshot.indexEntries.push_back({301, 1});
    snapshot.indexEntries.push_back({302, 1});

    DeviceRegistryEntry output = makeRecord(300, 30, 1, "output", "o");
    DeviceRegistryEntry firstOwner = makeRecord(301, 31, 1, "fade-a", "a");
    DeviceRegistryEntry secondOwner = makeRecord(302, 31, 1, "fade-b", "b");
    firstOwner.depCount = 1U;
    firstOwner.deps[0] = {DeviceRole::AnalogOutput, 300};
    secondOwner.depCount = 1U;
    secondOwner.deps[0] = {DeviceRole::AnalogOutput, 300};
    snapshot.records = {output, firstOwner, secondOwner};

    DeviceTypeRegistry types{};
    DeviceTypeDescriptor outputDescriptor{};
    outputDescriptor.typeId = 30;
    outputDescriptor.name = "AnalogOutput";
    outputDescriptor.currentConfigVersion = 1U;
    outputDescriptor.providedRoles = ProvidedRoles::of({DeviceRole::AnalogOutput});
    TEST_ASSERT_TRUE(types.registerDescriptor(outputDescriptor));

    DeviceTypeDescriptor ownerDescriptor{};
    ownerDescriptor.typeId = 31;
    ownerDescriptor.name = "ExclusiveOwner";
    ownerDescriptor.currentConfigVersion = 1U;
    ownerDescriptor.dependencyRequirements.push_back({DeviceRole::AnalogOutput, true});
    ownerDescriptor.exclusiveDependencyRoles = ProvidedRoles::of({DeviceRole::AnalogOutput});
    TEST_ASSERT_TRUE(types.registerDescriptor(ownerDescriptor));

    const DeviceValidationResult typed = DeviceRegistrySnapshotValidator::validateTypedRelationships(snapshot, &types);
    TEST_ASSERT_FALSE(typed.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(typed.error));
}
