#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"

#include <algorithm>
#include <unity.h>

using namespace ewfm;

namespace {
std::string encodeDummyConfig(const DummyDeviceConfigV2& config) {
    return ewfm::encodeDummyDeviceConfig(config);
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

struct CountingRuntime final : public IDeviceRuntime {
    explicit CountingRuntime(const DeviceRecord&) {}

    void begin(uint32_t now) override {
        beginCount += 1;
        beginNow = now;
        status_ = DeviceStatus::Starting;
    }
    void tickFastLoop(uint32_t now) override {
        tickFastLoopCount += 1;
        lastNow = now;
    }
    void tick100ms(uint32_t now) override {
        tick100msCount += 1;
        lastNow = now;
    }
    void tick1s(uint32_t now) override {
        tick1sCount += 1;
        lastNow = now;
    }
    void setParentRuntime(IDeviceRuntime* parentRuntime) override {
        if (parentRuntime_ == parentRuntime) {
            return;
        }
        parentRuntime_ = parentRuntime;
    }
    IDeviceRuntime* parentRuntime() const override {
        return parentRuntime_;
    }
    void attachChildRuntime(IDeviceRuntime* childRuntime) override {
        if (childRuntime == nullptr || hasChildRuntime(childRuntime)) {
            return;
        }
        childRuntimes_.push_back(childRuntime);
    }
    void detachChildRuntime(IDeviceRuntime* childRuntime) override {
        if (childRuntime == nullptr) {
            return;
        }
        const auto it = std::remove(childRuntimes_.begin(), childRuntimes_.end(), childRuntime);
        if (it != childRuntimes_.end()) {
            childRuntimes_.erase(it, childRuntimes_.end());
        }
    }
    const std::vector<IDeviceRuntime*>& childRuntimes() const override {
        return childRuntimes_;
    }
    void requestReconfigure() override {
        reconfigureCount += 1;
    }
    void requestDisable() override {
        disableCount += 1;
    }
    void requestDelete() override {
        deleteCount += 1;
    }
    DeviceStatus status() const override {
        return status_;
    }
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }

    bool hasChildRuntime(const IDeviceRuntime* childRuntime) const {
        return std::find(childRuntimes_.begin(), childRuntimes_.end(), childRuntime) != childRuntimes_.end();
    }

    uint32_t beginCount{0};
    uint32_t tickFastLoopCount{0};
    uint32_t tick100msCount{0};
    uint32_t tick1sCount{0};
    uint32_t reconfigureCount{0};
    uint32_t disableCount{0};
    uint32_t deleteCount{0};
    uint32_t beginNow{0};
    uint32_t lastNow{0};
    DeviceStatus status_{DeviceStatus::Unknown};
    IDeviceRuntime* parentRuntime_{nullptr};
    std::vector<IDeviceRuntime*> childRuntimes_{};
};

std::unique_ptr<IDeviceRuntime> createCountingRuntime(const DeviceRecord& record) {
    return std::unique_ptr<IDeviceRuntime>(new CountingRuntime(record));
}

DeviceTypeDescriptor makeChildDescriptor(DeviceTypeId typeId, DeviceTypeId parentTypeId, uint8_t maxChildren = 0) {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = typeId;
    descriptor.name = "ChildDevice";
    descriptor.currentConfigVersion = 1;
    descriptor.canHaveChildren = false;
    descriptor.maxChildren = maxChildren;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Immediate;
    descriptor.ticksFastLoop = false;
    descriptor.ticks100ms = false;
    descriptor.ticks1s = false;
    descriptor.compatibleParentTypes.push_back(parentTypeId);
    return descriptor;
}

DeviceRecord makeDummyRecord(DeviceId id, DeviceTypeId typeId, uint32_t configVersion, DeviceId parentId, bool hasParent,
                             const std::string& name, const DummyDeviceConfigV2& config) {
    DeviceRecord record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = id;
    record.header.typeId = typeId;
    record.header.configVersion = configVersion;
    record.header.configRevision = 1;
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

struct ToggleConfigStorage final : public IConfigStorage {
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
        if (failNextPutString_) {
            failNextPutString_ = false;
            return false;
        }
        return storage_.putString(key, value);
    }
    bool getString(const char* key, std::string& value) const override {
        return storage_.getString(key, value);
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
        if (failNextRemove_) {
            failNextRemove_ = false;
            return false;
        }
        return storage_.remove(key);
    }

    void failNextPutString() {
        failNextPutString_ = true;
    }
    void failNextRemove() {
        failNextRemove_ = true;
    }

    MemoryConfigStorage storage_;
    bool failNextPutString_{false};
    bool failNextRemove_{false};
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

    DeviceRegistrySnapshot createdSnapshot;
    TEST_ASSERT_TRUE(store.load(createdSnapshot, &types).ok());
    TEST_ASSERT_EQUAL_UINT32(1, createdSnapshot.records.size());
    TEST_ASSERT_EQUAL_STRING("dummy-a", createdSnapshot.records[0].name.c_str());

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

void test_registry_revisions_and_runtime_status_do_not_mix_with_config() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({111, 112});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult createResult = registry.create(makeDummyCreateRequest("revision-dummy"), 10);
    TEST_ASSERT_TRUE(createResult.ok());
    TEST_ASSERT_EQUAL_UINT32(111, createResult.deviceId);
    TEST_ASSERT_EQUAL_UINT32(1, registry.registryRevision());
    TEST_ASSERT_EQUAL_UINT32(1, registry.find(111)->header.configRevision);

    DeviceMutationResult renameResult = registry.rename(111, "revision-dummy-renamed", 20, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE(renameResult.ok());
    TEST_ASSERT_EQUAL_UINT32(2, registry.registryRevision());
    TEST_ASSERT_EQUAL_UINT32(1, registry.find(111)->header.configRevision);

    DummyDeviceConfigV2 updatedConfig{};
    updatedConfig.restorePreviousState = true;
    updatedConfig.defaultOutput = true;
    updatedConfig.currentOutput = true;
    updatedConfig.inverted = false;
    const std::string updatedPayload = encodeDummyConfig(updatedConfig);
    DeviceMutationResult updateResult =
        registry.updateConfig(111, updatedPayload, DummyDevice::descriptor().currentConfigVersion, 30, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE(updateResult.ok());
    TEST_ASSERT_EQUAL_UINT32(3, registry.registryRevision());
    TEST_ASSERT_EQUAL_UINT32(2, registry.find(111)->header.configRevision);

    DeviceCreateResult statusCreate = registry.create(makeDummyCreateRequest("status-dummy"), 40);
    TEST_ASSERT_TRUE(statusCreate.ok());
    TEST_ASSERT_EQUAL_UINT32(112, statusCreate.deviceId);
    TEST_ASSERT_EQUAL_UINT32(1, registry.find(112)->header.configRevision);

    const DeviceMutationResult commandResult =
        registry.command(DeviceCommand{DeviceCommandType::SetStatus, 112, "fault", DevicePersistencePolicy::Delayed}, 50);
    TEST_ASSERT_TRUE(commandResult.ok());
    TEST_ASSERT_EQUAL_UINT32(1, registry.find(112)->header.configRevision);
    IDeviceRuntime* runtime = registry.runtime(112);
    TEST_ASSERT_NOT_NULL(runtime);
    const int statusBeforeTick = static_cast<int>(runtime->status());
    registry.tickFastLoop(51);
    registry.tickFastLoop(52);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(runtime->status()));
    TEST_ASSERT_NOT_EQUAL(statusBeforeTick, static_cast<int>(runtime->status()));
    TEST_ASSERT_EQUAL_UINT32(1, registry.find(112)->header.configRevision);
}

void test_registry_duplicate_generated_id_retries() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({201, 201, 202});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("first"), 10).ok());
    DeviceCreateResult secondResult = registry.create(makeDummyCreateRequest("second"), 20);
    TEST_ASSERT_TRUE(secondResult.ok());
    TEST_ASSERT_EQUAL_UINT32(202, secondResult.deviceId);
    TEST_ASSERT_NOT_NULL(registry.find(201));
    TEST_ASSERT_NOT_NULL(registry.find(202));
}

void test_registry_rejects_invalid_device_id() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({301, 302});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceMutationResult renameResult = registry.rename(0, "invalid", 10, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_FALSE(renameResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::MissingRecord), static_cast<int>(renameResult.validation.error));
}

void test_registry_rejects_max_device_count() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DummyDeviceConfigV2 config{};
    config.restorePreviousState = true;
    config.defaultOutput = false;
    config.currentOutput = false;

    DeviceRegistrySnapshot snapshot;
    for (uint32_t index = 1; index <= kMaxDynamicDevices; ++index) {
        DeviceRecord record{};
        record.header.recordVersion = kDeviceRecordHeaderVersion;
        record.header.deviceId = index;
        record.header.typeId = 1;
        record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
        record.header.configRevision = 1;
        record.header.payloadLength = static_cast<uint32_t>(encodeDummyConfig(config).size());
        record.name = "device-" + std::to_string(index);
        record.enabled = true;
        record.status = DeviceStatus::Ready;
        record.configPayload = encodeDummyConfig(config);
        snapshot.indexEntries.push_back({index, 1});
        snapshot.records.push_back(record);
    }
    TEST_ASSERT_TRUE(store.save(snapshot).ok());

    FixedDeviceIdSource idSource({401});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult createResult = registry.create(makeDummyCreateRequest("overflow"), 10);
    TEST_ASSERT_FALSE(createResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::BoundsExceeded), static_cast<int>(createResult.validation.error));
}

void test_registry_validates_parent_child_graph_rules() {
    {
        MemoryConfigStorage storage;
        DeviceRegistryStore store(storage);
        TEST_ASSERT_TRUE(store.begin(false));

        FixedDeviceIdSource idSource({901});
        DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
        DeviceTypeDescriptor childDescriptor = makeChildDescriptor(55, 1);
        TEST_ASSERT_TRUE(types.registerDescriptor(childDescriptor));

        DeviceRegistry registry(store, types, idSource);
        TEST_ASSERT_TRUE(registry.begin(0).ok());

        DeviceCreateRequest missingParent = makeDummyCreateRequest("missing-parent");
        missingParent.typeId = 55;
        missingParent.configVersion = 1;
        missingParent.configPayload.clear();
        missingParent.parentDeviceId = 12345;
        missingParent.hasParent = true;
        DeviceCreateResult missingParentResult = registry.create(missingParent, 10);
        TEST_ASSERT_FALSE(missingParentResult.ok());
        TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(missingParentResult.validation.error));
    }

    {
        MemoryConfigStorage storage;
        DeviceRegistryStore store(storage);
        TEST_ASSERT_TRUE(store.begin(false));

        FixedDeviceIdSource idSource({902});
        DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
        DeviceTypeDescriptor childDescriptor = makeChildDescriptor(55, 1);
        TEST_ASSERT_TRUE(types.registerDescriptor(childDescriptor));

        DeviceRegistry registry(store, types, idSource);
        TEST_ASSERT_TRUE(registry.begin(0).ok());

        DeviceCreateRequest selfParent = makeDummyCreateRequest("self-parent");
        selfParent.typeId = 55;
        selfParent.configVersion = 1;
        selfParent.configPayload.clear();
        selfParent.hasParent = true;
        selfParent.parentDeviceId = 902;
        DeviceCreateResult selfParentResult = registry.create(selfParent, 20);
        TEST_ASSERT_FALSE(selfParentResult.ok());
        TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(selfParentResult.validation.error));
    }

    {
        MemoryConfigStorage storage;
        DeviceRegistryStore store(storage);
        TEST_ASSERT_TRUE(store.begin(false));

        FixedDeviceIdSource idSource({903, 904, 905});
        DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
        DeviceTypeDescriptor parentDescriptor;
        parentDescriptor.typeId = 57;
        parentDescriptor.name = "BusDevice";
        parentDescriptor.currentConfigVersion = 1;
        parentDescriptor.canHaveChildren = true;
        parentDescriptor.maxChildren = 1;
        parentDescriptor.createRuntime = nullptr;
        TEST_ASSERT_TRUE(types.registerDescriptor(parentDescriptor));
        DeviceTypeDescriptor incompatibleChildDescriptor = makeChildDescriptor(56, 58);
        TEST_ASSERT_TRUE(types.registerDescriptor(incompatibleChildDescriptor));
        DeviceTypeDescriptor childDescriptor = makeChildDescriptor(59, 57);
        TEST_ASSERT_TRUE(types.registerDescriptor(childDescriptor));

        DeviceRegistry registry(store, types, idSource);
        TEST_ASSERT_TRUE(registry.begin(0).ok());

        DeviceCreateRequest parentRequest = makeDummyCreateRequest("parent");
        parentRequest.typeId = 57;
        parentRequest.configPayload.clear();
        parentRequest.configVersion = 1;
        parentRequest.persistencePolicy = DevicePersistencePolicy::Immediate;
        DeviceCreateResult parentResult = registry.create(parentRequest, 30);
        TEST_ASSERT_TRUE(parentResult.ok());

        DeviceCreateRequest incompatible = makeDummyCreateRequest("incompatible");
        incompatible.typeId = 56;
        incompatible.configVersion = 1;
        incompatible.configPayload.clear();
        incompatible.hasParent = true;
        incompatible.parentDeviceId = parentResult.deviceId;
        DeviceCreateResult incompatibleResult = registry.create(incompatible, 40);
        TEST_ASSERT_FALSE(incompatibleResult.ok());
        TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(incompatibleResult.validation.error));

        DeviceCreateRequest childOne = makeDummyCreateRequest("child-one");
        childOne.typeId = 59;
        childOne.configVersion = 1;
        childOne.configPayload.clear();
        childOne.hasParent = true;
        childOne.parentDeviceId = parentResult.deviceId;
        DeviceCreateResult childOneResult = registry.create(childOne, 50);
        TEST_ASSERT_TRUE(childOneResult.ok());
    }

    {
        MemoryConfigStorage storage;
        DeviceRegistryStore store(storage);
        TEST_ASSERT_TRUE(store.begin(false));

        DummyDeviceConfigV2 config{};
        config.restorePreviousState = true;
        config.defaultOutput = false;
        config.currentOutput = false;

        DeviceRegistrySnapshot maxChildSnapshot;
        maxChildSnapshot.indexEntries.push_back({903, 57});
        maxChildSnapshot.indexEntries.push_back({904, 59});
        maxChildSnapshot.indexEntries.push_back({905, 59});
        maxChildSnapshot.records.push_back(makeDummyRecord(903, 57, 1, 0, false, "parent", config));
        maxChildSnapshot.records.push_back(makeDummyRecord(904, 59, 1, 903, true, "child-one", config));
        maxChildSnapshot.records.push_back(makeDummyRecord(905, 59, 1, 903, true, "child-two", config));
        TEST_ASSERT_TRUE(store.save(maxChildSnapshot).ok());

        FixedDeviceIdSource idSource({906});
        DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
        DeviceTypeDescriptor parentDescriptor;
        parentDescriptor.typeId = 57;
        parentDescriptor.name = "BusDevice";
        parentDescriptor.currentConfigVersion = 1;
        parentDescriptor.canHaveChildren = true;
        parentDescriptor.maxChildren = 1;
        parentDescriptor.createRuntime = nullptr;
        TEST_ASSERT_TRUE(types.registerDescriptor(parentDescriptor));
        DeviceTypeDescriptor childDescriptor = makeChildDescriptor(59, 57);
        TEST_ASSERT_TRUE(types.registerDescriptor(childDescriptor));

        DeviceRegistry registry(store, types, idSource);
        DeviceValidationResult beginResult = registry.begin(0);
        TEST_ASSERT_FALSE(beginResult.ok());
        TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(beginResult.error));
    }

    {
        MemoryConfigStorage storage;
        DeviceRegistryStore store(storage);
        TEST_ASSERT_TRUE(store.begin(false));

        DummyDeviceConfigV2 config{};
        config.restorePreviousState = true;
        config.defaultOutput = false;
        config.currentOutput = false;

        DeviceRegistrySnapshot cyclicSnapshot;
        cyclicSnapshot.indexEntries.push_back({904, 1});
        cyclicSnapshot.indexEntries.push_back({905, 1});
        cyclicSnapshot.records.push_back(
            makeDummyRecord(904, 1, DummyDevice::descriptor().currentConfigVersion, 905, true, "cycle-a", config));
        cyclicSnapshot.records.push_back(
            makeDummyRecord(905, 1, DummyDevice::descriptor().currentConfigVersion, 904, true, "cycle-b", config));
        TEST_ASSERT_FALSE(store.save(cyclicSnapshot).ok());
    }
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
    TEST_ASSERT_NOT_NULL(registry.runtime(201));
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(registry.runtime(201)->status()));

    DeviceMutationResult enableResult = registry.setEnabled(201, true, 30, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(enableResult.ok());
    TEST_ASSERT_NOT_NULL(registry.runtime(201));

    DeviceMutationResult deleteResult = registry.remove(201, 40, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(deleteResult.ok());
    TEST_ASSERT_NULL(registry.find(201));
    TEST_ASSERT_NULL(registry.runtime(201));
}

void test_registry_rejects_parent_delete_with_children() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({211, 212});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult parentResult = registry.create(makeDummyCreateRequest("parent"), 10);
    TEST_ASSERT_TRUE(parentResult.ok());

    DeviceCreateRequest childRequest = makeDummyCreateRequest("child");
    childRequest.hasParent = true;
    childRequest.parentDeviceId = parentResult.deviceId;
    DeviceCreateResult childResult = registry.create(childRequest, 20);
    TEST_ASSERT_TRUE(childResult.ok());

    DeviceMutationResult deleteResult = registry.remove(parentResult.deviceId, 30, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_FALSE(deleteResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(deleteResult.validation.error));
    TEST_ASSERT_EQUAL_UINT32(1, deleteResult.dependentChildDeviceIds.size());
    TEST_ASSERT_EQUAL_UINT32(childResult.deviceId, deleteResult.dependentChildDeviceIds[0]);
    TEST_ASSERT_NOT_NULL(registry.find(parentResult.deviceId));
    TEST_ASSERT_NOT_NULL(registry.find(childResult.deviceId));
    TEST_ASSERT_NOT_NULL(registry.runtime(parentResult.deviceId));
    TEST_ASSERT_NOT_NULL(registry.runtime(childResult.deviceId));
    TEST_ASSERT_EQUAL_PTR(registry.runtime(parentResult.deviceId), registry.runtime(childResult.deviceId)->parentRuntime());
    const auto& parentChildren = registry.runtime(parentResult.deviceId)->childRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, parentChildren.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(childResult.deviceId), parentChildren[0]);
}

void test_registry_reassigns_parent_atomically() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({231, 232, 233});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceTypeDescriptor childDescriptor;
    childDescriptor.typeId = 59;
    childDescriptor.name = "CountingChild";
    childDescriptor.currentConfigVersion = 1;
    childDescriptor.compatibleParentTypes.push_back(1);
    childDescriptor.createRuntime = &createCountingRuntime;
    TEST_ASSERT_TRUE(types.registerDescriptor(childDescriptor));
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult firstParent = registry.create(makeDummyCreateRequest("parent-a"), 10);
    TEST_ASSERT_TRUE(firstParent.ok());
    DeviceCreateResult secondParent = registry.create(makeDummyCreateRequest("parent-b"), 20);
    TEST_ASSERT_TRUE(secondParent.ok());

    DeviceCreateRequest childRequest = makeDummyCreateRequest("child");
    childRequest.typeId = 59;
    childRequest.hasParent = true;
    childRequest.parentDeviceId = firstParent.deviceId;
    DeviceCreateResult childResult = registry.create(childRequest, 30);
    TEST_ASSERT_TRUE(childResult.ok());

    auto* childRuntime = dynamic_cast<CountingRuntime*>(registry.runtime(childResult.deviceId));
    TEST_ASSERT_NOT_NULL(childRuntime);
    TEST_ASSERT_EQUAL_UINT32(1, childRuntime->beginCount);
    TEST_ASSERT_EQUAL_UINT32(0, childRuntime->reconfigureCount);

    DeviceMutationResult reparentResult =
        registry.setParent(childResult.deviceId, true, secondParent.deviceId, 40, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(reparentResult.ok());
    TEST_ASSERT_FALSE(reparentResult.pendingPersistence);
    TEST_ASSERT_EQUAL_UINT32(secondParent.deviceId, registry.find(childResult.deviceId)->parentDeviceId);
    auto* sameChildRuntime = dynamic_cast<CountingRuntime*>(registry.runtime(childResult.deviceId));
    TEST_ASSERT_NOT_NULL(sameChildRuntime);
    TEST_ASSERT_TRUE(sameChildRuntime == childRuntime);
    TEST_ASSERT_EQUAL_UINT32(1, sameChildRuntime->beginCount);
    TEST_ASSERT_EQUAL_UINT32(1, sameChildRuntime->reconfigureCount);
    TEST_ASSERT_EQUAL_PTR(registry.runtime(secondParent.deviceId), sameChildRuntime->parentRuntime());
    const auto& firstParentChildren = registry.runtime(firstParent.deviceId)->childRuntimes();
    TEST_ASSERT_EQUAL_UINT32(0, firstParentChildren.size());
    const auto& secondParentChildren = registry.runtime(secondParent.deviceId)->childRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, secondParentChildren.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(childResult.deviceId), secondParentChildren[0]);
    TEST_ASSERT_TRUE(registry.remove(firstParent.deviceId, 50, DevicePersistencePolicy::Immediate).ok());

    DeviceRegistrySnapshot loaded;
    TEST_ASSERT_TRUE(store.load(loaded, &types).ok());
    const auto loadedChild =
        std::find_if(loaded.records.begin(), loaded.records.end(),
                     [childId = childResult.deviceId](const DeviceRecord& record) { return record.header.deviceId == childId; });
    TEST_ASSERT_TRUE(loadedChild != loaded.records.end());
    TEST_ASSERT_EQUAL_UINT32(secondParent.deviceId, loadedChild->parentDeviceId);

    ToggleConfigStorage failingStorage;
    DeviceRegistryStore failingStore(failingStorage);
    TEST_ASSERT_TRUE(failingStore.begin(false));
    failingStorage.failNextPutString();

    FixedDeviceIdSource failingIdSource({241, 242, 243});
    DeviceRegistry failingRegistry(failingStore, types, failingIdSource);
    TEST_ASSERT_TRUE(failingRegistry.begin(0).ok());

    DeviceCreateResult failingParentA = failingRegistry.create(makeDummyCreateRequest("parent-c"), 10);
    TEST_ASSERT_TRUE(failingParentA.ok());
    DeviceCreateResult failingParentB = failingRegistry.create(makeDummyCreateRequest("parent-d"), 20);
    TEST_ASSERT_TRUE(failingParentB.ok());

    DeviceCreateRequest failingChild = makeDummyCreateRequest("child-fail");
    failingChild.hasParent = true;
    failingChild.parentDeviceId = failingParentA.deviceId;
    DeviceCreateResult failingChildResult = failingRegistry.create(failingChild, 30);
    TEST_ASSERT_TRUE(failingChildResult.ok());

    DeviceMutationResult failingReparent =
        failingRegistry.setParent(failingChildResult.deviceId, true, failingParentB.deviceId, 40, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_FALSE(failingReparent.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::StorageError), static_cast<int>(failingReparent.validation.error));
    TEST_ASSERT_EQUAL_UINT32(failingParentA.deviceId, failingRegistry.find(failingChildResult.deviceId)->parentDeviceId);
}

void test_registry_propagates_parent_dependency_status_and_recovers() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({221, 222});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult parentResult = registry.create(makeDummyCreateRequest("parent"), 10);
    TEST_ASSERT_TRUE(parentResult.ok());

    DeviceCreateRequest childRequest = makeDummyCreateRequest("child");
    childRequest.hasParent = true;
    childRequest.parentDeviceId = parentResult.deviceId;
    DeviceCreateResult childResult = registry.create(childRequest, 20);
    TEST_ASSERT_TRUE(childResult.ok());
    TEST_ASSERT_NOT_NULL(registry.runtime(childResult.deviceId));

    DeviceMutationResult disableResult = registry.setEnabled(parentResult.deviceId, false, 30, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(disableResult.ok());
    TEST_ASSERT_NOT_NULL(registry.runtime(childResult.deviceId));
    auto disabledRecords = registry.list();
    const auto disabledChild =
        std::find_if(disabledRecords.begin(), disabledRecords.end(),
                     [childId = childResult.deviceId](const DeviceRecord& record) { return record.header.deviceId == childId; });
    TEST_ASSERT_TRUE(disabledChild != disabledRecords.end());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(disabledChild->status));
    TEST_ASSERT_EQUAL_PTR(registry.runtime(parentResult.deviceId), registry.runtime(childResult.deviceId)->parentRuntime());
    const auto& blockedParentChildren = registry.runtime(parentResult.deviceId)->childRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, blockedParentChildren.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(childResult.deviceId), blockedParentChildren[0]);

    DeviceMutationResult enableResult = registry.setEnabled(parentResult.deviceId, true, 40, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(enableResult.ok());
    registry.tickFastLoop(41);
    registry.tickFastLoop(42);
    registry.tickFastLoop(43);
    registry.tickFastLoop(44);
    TEST_ASSERT_NOT_NULL(registry.runtime(childResult.deviceId));
    auto recoveredRecords = registry.list();
    const auto recoveredChild =
        std::find_if(recoveredRecords.begin(), recoveredRecords.end(),
                     [childId = childResult.deviceId](const DeviceRecord& record) { return record.header.deviceId == childId; });
    TEST_ASSERT_TRUE(recoveredChild != recoveredRecords.end());
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(recoveredChild->status));
    const auto& recoveredParentChildren = registry.runtime(parentResult.deviceId)->childRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, recoveredParentChildren.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(childResult.deviceId), recoveredParentChildren[0]);

    DeviceMutationResult faultResult =
        registry.command(DeviceCommand{DeviceCommandType::SetStatus, parentResult.deviceId, "fault", DevicePersistencePolicy::Delayed}, 50);
    TEST_ASSERT_TRUE(faultResult.ok());
    registry.tickFastLoop(51);
    registry.tickFastLoop(52);
    TEST_ASSERT_NOT_NULL(registry.runtime(childResult.deviceId));
    auto faultedRecords = registry.list();
    const auto faultedChild =
        std::find_if(faultedRecords.begin(), faultedRecords.end(),
                     [childId = childResult.deviceId](const DeviceRecord& record) { return record.header.deviceId == childId; });
    TEST_ASSERT_TRUE(faultedChild != faultedRecords.end());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(faultedChild->status));
}

void test_registry_set_parent_command_normalization() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({311, 312, 313});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult parentA = registry.create(makeDummyCreateRequest("parent-a"), 10);
    DeviceCreateResult parentB = registry.create(makeDummyCreateRequest("parent-b"), 20);
    TEST_ASSERT_TRUE(parentA.ok());
    TEST_ASSERT_TRUE(parentB.ok());

    DeviceCreateRequest childRequest = makeDummyCreateRequest("child");
    childRequest.hasParent = true;
    childRequest.parentDeviceId = parentA.deviceId;
    DeviceCreateResult child = registry.create(childRequest, 30);
    TEST_ASSERT_TRUE(child.ok());

    DeviceMutationResult commandResult =
        registry.command(DeviceCommand{DeviceCommandType::SetParent, child.deviceId, "parent=312", DevicePersistencePolicy::Immediate}, 40);
    TEST_ASSERT_TRUE(commandResult.ok());
    TEST_ASSERT_EQUAL_UINT32(parentB.deviceId, registry.find(child.deviceId)->parentDeviceId);
    TEST_ASSERT_EQUAL_PTR(registry.runtime(parentB.deviceId), registry.runtime(child.deviceId)->parentRuntime());

    DeviceMutationResult clearParentResult =
        registry.command(DeviceCommand{DeviceCommandType::SetParent, child.deviceId, "parent=0", DevicePersistencePolicy::Immediate}, 41);
    TEST_ASSERT_TRUE(clearParentResult.ok());
    TEST_ASSERT_FALSE(registry.find(child.deviceId)->hasParent);
    TEST_ASSERT_NULL(registry.runtime(child.deviceId)->parentRuntime());

    DeviceMutationResult invalidPayload =
        registry.command(DeviceCommand{DeviceCommandType::SetParent, child.deviceId, "invalid", DevicePersistencePolicy::Immediate}, 42);
    TEST_ASSERT_FALSE(invalidPayload.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidCommand), static_cast<int>(invalidPayload.validation.error));
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

void test_registry_invokes_only_declared_cadences() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({551, 552});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = 55;
    descriptor.name = "CountingDevice";
    descriptor.currentConfigVersion = 1;
    descriptor.ticks100ms = true;
    descriptor.createRuntime = &createCountingRuntime;
    TEST_ASSERT_TRUE(types.registerDescriptor(descriptor));

    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateRequest request{};
    request.typeId = 55;
    request.name = "counting";
    request.configVersion = 1;
    request.configPayload = "x";
    request.persistencePolicy = DevicePersistencePolicy::Immediate;

    DeviceCreateResult createResult = registry.create(request, 10);
    TEST_ASSERT_TRUE(createResult.ok());
    auto* runtime = dynamic_cast<CountingRuntime*>(registry.runtime(createResult.deviceId));
    TEST_ASSERT_NOT_NULL(runtime);
    TEST_ASSERT_EQUAL_UINT32(1, runtime->beginCount);
    TEST_ASSERT_EQUAL_UINT32(10, runtime->beginNow);

    registry.tickFastLoop(20);
    registry.tick1s(30);
    TEST_ASSERT_EQUAL_UINT32(0, runtime->tickFastLoopCount);
    TEST_ASSERT_EQUAL_UINT32(0, runtime->tick1sCount);

    registry.tick100ms(40);
    TEST_ASSERT_EQUAL_UINT32(1, runtime->tick100msCount);
    TEST_ASSERT_EQUAL_UINT32(40, runtime->lastNow);
}

void test_registry_immediate_persistence_failure_rolls_back_create() {
    ToggleConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    storage.failNextPutString();

    FixedDeviceIdSource idSource({601});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult createResult = registry.create(makeDummyCreateRequest("rollback"), 10);
    TEST_ASSERT_FALSE(createResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::StorageError), static_cast<int>(createResult.validation.error));
    TEST_ASSERT_NULL(registry.find(601));
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());
    TEST_ASSERT_EQUAL_UINT32(0, registry.registryRevision());
}

void test_registry_delayed_dirty_state_and_forced_flush() {
    ToggleConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({701, 702});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("dirty-dummy"), 10).ok());

    DeviceMutationResult renameResult = registry.rename(701, "dirty-dummy-2", 20, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE(renameResult.ok());
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
    TEST_ASSERT_TRUE(registry.dirtyIndex());
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyConfigRecordIds().size());
    TEST_ASSERT_EQUAL_UINT32(20, registry.firstDirtyAt());
    TEST_ASSERT_EQUAL_UINT32(20, registry.lastChangeAt());

    storage.failNextPutString();
    DeviceValidationResult flushResult = registry.flushNow();
    TEST_ASSERT_FALSE(flushResult.ok());
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
    TEST_ASSERT_TRUE(registry.dirtyIndex());
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyConfigRecordIds().size());

    TEST_ASSERT_TRUE(registry.flushNow().ok());
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());

    DeviceRegistrySnapshot loaded;
    TEST_ASSERT_TRUE(store.load(loaded, &types).ok());
    TEST_ASSERT_EQUAL_STRING("dirty-dummy-2", loaded.records[0].name.c_str());
}

void test_registry_max_delay_flushes_after_repeated_dirty_updates() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({801, 802});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("delay-dummy"), 10).ok());

    TEST_ASSERT_TRUE(registry.rename(801, "delay-dummy-2", 20, DevicePersistencePolicy::Delayed).ok());
    TEST_ASSERT_TRUE(registry.rename(801, "delay-dummy-3", 1700, DevicePersistencePolicy::Delayed).ok());
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
    TEST_ASSERT_EQUAL_UINT32(20, registry.firstDirtyAt());
    TEST_ASSERT_EQUAL_UINT32(1700, registry.lastChangeAt());

    registry.tick(1900);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());

    registry.tick(2105);
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());

    DeviceRegistrySnapshot loaded;
    TEST_ASSERT_TRUE(store.load(loaded, &types).ok());
    TEST_ASSERT_EQUAL_STRING("delay-dummy-3", loaded.records[0].name.c_str());
}

void test_registry_coalesces_retained_state_updates() {
    MemoryConfigStorage storage;
    DeviceRegistryStore registryStore(storage);
    RetainedStateStore retainedStore(storage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    FixedDeviceIdSource idSource({501, 502});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource, &retainedStore);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("retained-dummy"), 10).ok());

    DeviceMutationResult firstResult = registry.setRetainedState(501, "output=0", 20, DevicePersistencePolicy::Coalesced);
    TEST_ASSERT_TRUE(firstResult.ok());
    TEST_ASSERT_TRUE(firstResult.pendingPersistence);
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyRetainedStateIds().size());
    TEST_ASSERT_EQUAL_UINT32(20, registry.firstDirtyAt());
    TEST_ASSERT_EQUAL_UINT32(20, registry.lastChangeAt());

    DeviceMutationResult secondResult = registry.setRetainedState(501, "output=1", 40, DevicePersistencePolicy::Coalesced);
    TEST_ASSERT_TRUE(secondResult.ok());
    TEST_ASSERT_TRUE(secondResult.pendingPersistence);
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyRetainedStateIds().size());
    TEST_ASSERT_EQUAL_UINT32(20, registry.firstDirtyAt());
    TEST_ASSERT_EQUAL_UINT32(40, registry.lastChangeAt());

    TEST_ASSERT_TRUE(registry.flushNow().ok());
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());

    RetainedStateRecord loaded{};
    DeviceValidationResult loadResult = retainedStore.load(501, loaded);
    TEST_ASSERT_TRUE(loadResult.ok());
    TEST_ASSERT_EQUAL_STRING("output=1", loaded.payload.c_str());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_registry_begin_loads_runtime_devices);
    RUN_TEST(test_registry_create_rename_and_flush);
    RUN_TEST(test_registry_disable_enable_and_delete);
    RUN_TEST(test_registry_rejects_duplicate_name_and_unsupported_type);
    RUN_TEST(test_registry_delayed_flushes_on_tick);
    RUN_TEST(test_registry_revisions_and_runtime_status_do_not_mix_with_config);
    RUN_TEST(test_registry_duplicate_generated_id_retries);
    RUN_TEST(test_registry_rejects_invalid_device_id);
    RUN_TEST(test_registry_rejects_max_device_count);
    RUN_TEST(test_registry_validates_parent_child_graph_rules);
    RUN_TEST(test_registry_rejects_parent_delete_with_children);
    RUN_TEST(test_registry_propagates_parent_dependency_status_and_recovers);
    RUN_TEST(test_registry_set_parent_command_normalization);
    RUN_TEST(test_registry_invokes_only_declared_cadences);
    RUN_TEST(test_registry_coalesces_retained_state_updates);
    return UNITY_END();
}
