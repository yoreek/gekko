#include "config/MemoryConfigStorage.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "integrations/common/DeviceEventDispatcher.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
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
    CountingRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
        bindDeviceIdentity(record, configBlob);
    }

    void begin(uint32_t now) override {
        beginCount += 1;
        beginNow = now;
        status_ = DeviceStatus::Ready;
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
    void setDependencyRuntime(DeviceDependencyRole role, IDeviceRuntime* dependencyRuntime) override {
        for (uint8_t index = 0; index < dependencyCount_; ++index) {
            if (dependencyLinks_[index].role == role) {
                dependencyRuntimes_[index] = dependencyRuntime;
                return;
            }
        }
    }
    IDeviceRuntime* dependencyRuntime(DeviceDependencyRole role) const override {
        for (uint8_t index = 0; index < dependencyCount_; ++index) {
            if (dependencyLinks_[index].role == role) {
                return dependencyRuntimes_[index];
            }
        }
        return nullptr;
    }
    uint8_t dependencyCount() const override {
        return dependencyCount_;
    }
    const DeviceDependencyLink* dependencyLinks() const override {
        return dependencyLinks_.data();
    }
    bool hasDependencies() const override {
        return dependencyCount_ > 0;
    }
    DeviceId dependencyDeviceId(DeviceDependencyRole role) const override {
        for (uint8_t index = 0; index < dependencyCount_; ++index) {
            if (dependencyLinks_[index].role == role) {
                return dependencyLinks_[index].deviceId;
            }
        }
        return 0;
    }
    void attachDependentRuntime(IDeviceRuntime* childRuntime) override {
        if (childRuntime == nullptr || hasDependentRuntime(childRuntime)) {
            return;
        }
        dependentRuntimes_.push_back(childRuntime);
    }
    void detachDependentRuntime(IDeviceRuntime* childRuntime) override {
        if (childRuntime == nullptr) {
            return;
        }
        const auto it = std::remove(dependentRuntimes_.begin(), dependentRuntimes_.end(), childRuntime);
        if (it != dependentRuntimes_.end()) {
            dependentRuntimes_.erase(it, dependentRuntimes_.end());
        }
    }
    const std::vector<IDeviceRuntime*>& dependentRuntimes() const override {
        return dependentRuntimes_;
    }
    void requestReconfigure() override {
        reconfigureCount += 1;
        status_ = DeviceStatus::Reconfiguring;
    }
    void requestDisable() override {
        disableCount += 1;
        status_ = DeviceStatus::Disabled;
    }
    void requestDelete() override {
        deleteCount += 1;
        status_ = DeviceStatus::Deleting;
    }
    DeviceStatus status() const override {
        return status_;
    }
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override {
        DeviceBaseConfigV1 base{};
        TEST_ASSERT_TRUE(readDeviceBaseConfig(config, base));
        deviceId_ = record.header.deviceId;
        typeId_ = record.header.typeId;
        configVersion_ = record.header.configVersion;
        configRevision_ = record.header.configRevision;
        dependencyCount_ = record.dependencyCount();
        for (uint8_t index = 0; index < dependencyCount_ && index < kMaxDeviceDependencies; ++index) {
            dependencyLinks_[index] = record.dependencyLinks()[index];
            dependencyRuntimes_[index] = nullptr;
        }
        persistencePolicy_ = record.persistencePolicy;
        enabled_ = base.enabled != 0U;
        std::snprintf(name_, sizeof(name_), "%s", base.name);
        configBlob_ = config;
    }
    DeviceId deviceId() const override {
        return deviceId_;
    }
    DeviceTypeId typeId() const override {
        return typeId_;
    }
    uint32_t configVersion() const override {
        return configVersion_;
    }
    uint32_t configRevision() const override {
        return configRevision_;
    }
    bool enabled() const override {
        return enabled_;
    }
    const char* name() const override {
        return name_;
    }
    DevicePersistencePolicy persistencePolicy() const override {
        return persistencePolicy_;
    }
    bool handleCommand(const DeviceCommand& command) override {
        if (command.type == DeviceCommandType::SetStatus) {
            if (command.payload.equals("fault")) {
                status_ = DeviceStatus::Faulted;
                return true;
            }
            if (command.payload.equals("ready")) {
                status_ = DeviceStatus::Ready;
                return true;
            }
        }
        return false;
    }
    bool serializeConfigBlob(DeviceConfigBlob& configBlob) const override {
        configBlob = configBlob_;
        return true;
    }
    bool replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const override {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        const size_t size = deviceBaseConfigSize(baseConfig);
        return encodeDeviceBaseConfig(baseConfig, buffer, size) && configBlob.assign(buffer, size);
    }

    bool hasDependentRuntime(const IDeviceRuntime* childRuntime) const {
        return std::find(dependentRuntimes_.begin(), dependentRuntimes_.end(), childRuntime) != dependentRuntimes_.end();
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
    DeviceId deviceId_{0};
    DeviceTypeId typeId_{0};
    uint32_t configVersion_{0};
    uint32_t configRevision_{0};
    std::array<DeviceDependencyLink, kMaxDeviceDependencies> dependencyLinks_{};
    uint8_t dependencyCount_{0};
    std::array<IDeviceRuntime*, kMaxDeviceDependencies> dependencyRuntimes_{};
    bool enabled_{true};
    char name_[kMaxDeviceBaseNameLength + 1]{};
    DevicePersistencePolicy persistencePolicy_{DevicePersistencePolicy::Delayed};
    DeviceConfigBlob configBlob_{};
    std::vector<IDeviceRuntime*> dependentRuntimes_{};
};

std::unique_ptr<IDeviceRuntime> createCountingRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new CountingRuntime(record, configBlob));
}

DeviceTypeDescriptor makeDependentDescriptor(DeviceTypeId typeId, DeviceTypeId dependencyTypeId, uint8_t maxDependents = 0) {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = typeId;
    descriptor.name = "DependentDevice";
    descriptor.currentConfigVersion = 1;
    descriptor.maxDependents = maxDependents;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Immediate;
    descriptor.ticksFastLoop = false;
    descriptor.ticks100ms = false;
    descriptor.ticks1s = false;
    descriptor.dependencyRequirements.push_back({DeviceDependencyRole::OneWireBus, true, {dependencyTypeId}});
    return descriptor;
}

DeviceTypeDescriptor makeCommandableDescriptor(DeviceTypeId typeId) {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = typeId;
    descriptor.name = "CommandableDevice";
    descriptor.currentConfigVersion = 1;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Immediate;
    descriptor.ticksFastLoop = false;
    descriptor.ticks100ms = false;
    descriptor.ticks1s = false;
    descriptor.createRuntime = &createCountingRuntime;
    return descriptor;
}

DeviceTypeDescriptor makeRetainedDescriptor(DeviceTypeId typeId) {
    DeviceTypeDescriptor descriptor = makeCommandableDescriptor(typeId);
    descriptor.name = "RetainedDevice";
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = true;
    return descriptor;
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
    bool putBlob(const char* key, const uint8_t* value, size_t size) override {
        if (failNextPutString_) {
            failNextPutString_ = false;
            return false;
        }
        return storage_.putBlob(key, value, size);
    }
    bool getBlob(const char* key, uint8_t* value, size_t& size) const override {
        return storage_.getBlob(key, value, size);
    }
    bool putBlob(const char* key, const std::vector<uint8_t>& value) override {
        if (failNextPutString_) {
            failNextPutString_ = false;
            return false;
        }
        return storage_.putBlob(key, value);
    }
    bool getBlob(const char* key, std::vector<uint8_t>& value) const override {
        return storage_.getBlob(key, value);
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
    bool clear() override {
        return storage_.clear();
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

struct RecordingConfigStorage final : public IConfigStorage {
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
        putStringKeys.push_back(key);
        if (shouldFail(key)) {
            return false;
        }
        return storage_.putString(key, value);
    }
    bool getString(const char* key, std::string& value) const override {
        return storage_.getString(key, value);
    }
    bool putBlob(const char* key, const uint8_t* value, size_t size) override {
        putBlobKeys.push_back(key);
        putBlobSizes[key] = size;
        if (shouldFail(key)) {
            return false;
        }
        return storage_.putBlob(key, value, size);
    }
    bool getBlob(const char* key, uint8_t* value, size_t& size) const override {
        return storage_.getBlob(key, value, size);
    }
    bool putBlob(const char* key, const std::vector<uint8_t>& value) override {
        return putBlob(key, value.data(), value.size());
    }
    bool getBlob(const char* key, std::vector<uint8_t>& value) const override {
        return storage_.getBlob(key, value);
    }
    bool putUInt(const char* key, uint32_t value) override {
        putUIntKeys.push_back(key);
        if (shouldFail(key)) {
            return false;
        }
        return storage_.putUInt(key, value);
    }
    bool getUInt(const char* key, uint32_t& value) const override {
        return storage_.getUInt(key, value);
    }
    bool putBool(const char* key, bool value) override {
        putBoolKeys.push_back(key);
        if (shouldFail(key)) {
            return false;
        }
        return storage_.putBool(key, value);
    }
    bool getBool(const char* key, bool& value) const override {
        return storage_.getBool(key, value);
    }
    bool remove(const char* key) override {
        removeKeys.push_back(key);
        if (shouldFail(key)) {
            return false;
        }
        return storage_.remove(key);
    }
    bool clear() override {
        clearCount += 1;
        return storage_.clear();
    }

    void failNextWriteFor(const char* key) {
        failKey = key;
    }
    void clearLog() {
        putStringKeys.clear();
        putBlobKeys.clear();
        putBlobSizes.clear();
        putUIntKeys.clear();
        putBoolKeys.clear();
        removeKeys.clear();
        clearCount = 0;
    }
    size_t countBlobWrites(const char* key) const {
        return static_cast<size_t>(std::count(putBlobKeys.begin(), putBlobKeys.end(), std::string(key)));
    }
    size_t countUIntWrites(const char* key) const {
        return static_cast<size_t>(std::count(putUIntKeys.begin(), putUIntKeys.end(), std::string(key)));
    }
    bool removed(const char* key) const {
        return std::find(removeKeys.begin(), removeKeys.end(), std::string(key)) != removeKeys.end();
    }

    bool shouldFail(const char* key) {
        if (!failKey.empty() && failKey == key) {
            failKey.clear();
            return true;
        }
        return false;
    }

    MemoryConfigStorage storage_;
    std::string failKey{};
    std::vector<std::string> putStringKeys{};
    std::vector<std::string> putBlobKeys{};
    std::map<std::string, size_t> putBlobSizes{};
    std::vector<std::string> putUIntKeys{};
    std::vector<std::string> putBoolKeys{};
    std::vector<std::string> removeKeys{};
    uint32_t clearCount{0};
};

struct RecordingSink final : public IDeviceEventSink {
    void onDeviceEvent(const DeviceEvent& event) override {
        events.push_back(event);
    }

    void tickFastLoop(uint32_t now) override {
        lastTickNow = now;
    }
    void tick100ms(uint32_t now) override {
        lastTickNow = now;
    }
    void tick1s(uint32_t now) override {
        lastTickNow = now;
    }

    std::vector<DeviceEvent> events{};
    uint32_t lastTickNow{0};
};

bool hasEventKind(const std::vector<DeviceEvent>& events, DeviceEventKind kind) {
    return std::find_if(events.begin(), events.end(), [kind](const DeviceEvent& event) { return event.kind == kind; }) != events.end();
}

DeviceCreateRequest makeDummyCreateRequest(const std::string& name, bool enabled = true) {
    DummyDeviceConfigV1 config{};
    config.enabled = enabled;
    std::snprintf(config.name, sizeof(config.name), "%s", name.c_str());

    DeviceCreateRequest request{};
    request.typeId = 1;
    request.name = name;
    request.enabled = enabled;
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    request.configBlob = encodeDummyConfig(config);
    request.persistencePolicy = DevicePersistencePolicy::Immediate;
    return request;
}

void assertDummyConfigName(const DeviceConfigBlobMap& configBlobs, DeviceId deviceId, const char* expected) {
    const auto it = configBlobs.find(deviceId);
    TEST_ASSERT_TRUE(it != configBlobs.end());
    DummyDeviceConfigV1 config{};
    TEST_ASSERT_TRUE(decodeDummyDeviceConfig(it->second.data(), it->second.size(), config));
    TEST_ASSERT_EQUAL_STRING(expected, config.name);
}

void makeRecordKey(DeviceId deviceId, char (&out)[32]) {
    std::snprintf(out, sizeof(out), "record_%08x", static_cast<unsigned>(deviceId));
}

} // namespace

void test_registry_begin_loads_runtime_devices() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DeviceRegistrySnapshot snapshot;
    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = 11;
    record.header.typeId = 1;
    record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1;
    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "dummy");
    record.header.payloadLength = static_cast<uint32_t>(encodeDummyConfig(config).size());
    record.status = DeviceStatus::Ready;
    snapshot.indexEntries.push_back({11, 1});
    snapshot.records.push_back(record);
    DeviceConfigBlobMap configBlobs;
    configBlobs[11] = encodeDummyConfig(config);
    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());

    FixedDeviceIdSource idSource({21, 22});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);

    TEST_ASSERT_TRUE(registry.begin(100).ok());
    TEST_ASSERT_EQUAL_UINT32(1, registry.list().size());
    TEST_ASSERT_NOT_NULL(registry.runtime(11));
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
    TEST_ASSERT_NOT_NULL(registry.runtime(101));
    TEST_ASSERT_NOT_NULL(registry.runtime(101));

    DeviceRegistrySnapshot createdSnapshot;
    DeviceConfigBlobMap createdConfigBlobs;
    TEST_ASSERT_TRUE(store.load(createdSnapshot, createdConfigBlobs, &types).ok());
    TEST_ASSERT_EQUAL_UINT32(1, createdSnapshot.records.size());
    assertDummyConfigName(createdConfigBlobs, 101, "dummy-a");

    DeviceMutationResult renameResult = registry.rename(101, "dummy-b", 20, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE(renameResult.ok());
    TEST_ASSERT_TRUE(renameResult.pendingPersistence);
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
    TEST_ASSERT_EQUAL_STRING("dummy-b", registry.runtime(101)->name());

    TEST_ASSERT_TRUE(registry.flushNow().ok());
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());

    DeviceRegistrySnapshot loaded;
    DeviceConfigBlobMap loadedConfigBlobs;
    TEST_ASSERT_TRUE(store.load(loaded, loadedConfigBlobs, &types).ok());
    TEST_ASSERT_EQUAL_UINT32(1, loaded.records.size());
    assertDummyConfigName(loadedConfigBlobs, 101, "dummy-b");
}

void test_registry_revisions_and_runtime_status_do_not_mix_with_config() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({111, 112});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_TRUE(types.registerDescriptor(makeCommandableDescriptor(58)));
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult createResult = registry.create(makeDummyCreateRequest("revision-dummy"), 10);
    TEST_ASSERT_TRUE(createResult.ok());
    TEST_ASSERT_EQUAL_UINT32(111, createResult.deviceId);
    TEST_ASSERT_EQUAL_UINT32(1, registry.registryRevision());
    TEST_ASSERT_EQUAL_UINT32(1, registry.runtime(111)->configRevision());

    DeviceMutationResult renameResult = registry.rename(111, "revision-dummy-renamed", 20, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE(renameResult.ok());
    TEST_ASSERT_EQUAL_UINT32(2, registry.registryRevision());
    TEST_ASSERT_EQUAL_UINT32(1, registry.runtime(111)->configRevision());

    DummyDeviceConfigV1 updatedConfig{};
    updatedConfig.enabled = true;
    std::snprintf(updatedConfig.name, sizeof(updatedConfig.name), "%s", "revision-dummy");
    const BoundedBlob<kMaxDeviceConfigBytes> updatedPayload = encodeDummyConfig(updatedConfig);
    DeviceMutationResult updateResult =
        registry.updateConfig(111, updatedPayload, DummyDevice::descriptor().currentConfigVersion, 30, DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE(updateResult.ok());
    TEST_ASSERT_EQUAL_UINT32(3, registry.registryRevision());
    TEST_ASSERT_EQUAL_UINT32(2, registry.runtime(111)->configRevision());

    DeviceCreateRequest statusRequest = makeDummyCreateRequest("status-dummy");
    statusRequest.typeId = 58;
    DeviceCreateResult statusCreate = registry.create(statusRequest, 40);
    TEST_ASSERT_TRUE(statusCreate.ok());
    TEST_ASSERT_EQUAL_UINT32(112, statusCreate.deviceId);
    TEST_ASSERT_EQUAL_UINT32(1, registry.runtime(112)->configRevision());

    IDeviceRuntime* runtime = registry.runtime(112);
    TEST_ASSERT_NOT_NULL(runtime);
    const int statusBeforeCommand = static_cast<int>(runtime->status());
    const DeviceMutationResult commandResult =
        registry.command(DeviceCommand{DeviceCommandType::SetStatus, 112, "fault", DevicePersistencePolicy::Delayed}, 50);
    TEST_ASSERT_TRUE(commandResult.ok());
    TEST_ASSERT_EQUAL_UINT32(1, registry.runtime(112)->configRevision());
    registry.tickFastLoop(51);
    registry.tickFastLoop(52);
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Faulted), static_cast<int>(runtime->status()));
    TEST_ASSERT_NOT_EQUAL(statusBeforeCommand, static_cast<int>(runtime->status()));
    TEST_ASSERT_EQUAL_UINT32(1, registry.runtime(112)->configRevision());
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
    TEST_ASSERT_NOT_NULL(registry.runtime(201));
    TEST_ASSERT_NOT_NULL(registry.runtime(202));
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

    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "device");

    DeviceRegistrySnapshot snapshot;
    for (uint32_t index = 1; index <= kMaxDynamicDevices; ++index) {
        DeviceRegistryEntry record{};
        record.header.recordVersion = kDeviceRecordHeaderVersion;
        record.header.deviceId = index;
        record.header.typeId = 1;
        record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
        record.header.configRevision = 1;
        record.header.payloadLength = static_cast<uint32_t>(encodeDummyConfig(config).size());
        record.status = DeviceStatus::Ready;
        snapshot.indexEntries.push_back({index, 1});
        snapshot.records.push_back(record);
    }
    DeviceConfigBlobMap configBlobs;
    for (uint32_t index = 1; index <= kMaxDynamicDevices; ++index) {
        configBlobs[index] = encodeDummyConfig(config);
    }
    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());

    FixedDeviceIdSource idSource({401});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult createResult = registry.create(makeDummyCreateRequest("overflow"), 10);
    TEST_ASSERT_FALSE(createResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::BoundsExceeded), static_cast<int>(createResult.validation.error));
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
    TEST_ASSERT_NULL(registry.runtime(201));
    TEST_ASSERT_NULL(registry.runtime(201));
}

void test_registry_rejects_dependency_delete_with_dependents() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({211, 212});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult dependencyResult = registry.create(makeDummyCreateRequest("dependency"), 10);
    TEST_ASSERT_TRUE(dependencyResult.ok());

    DeviceCreateRequest dependentRequest = makeDummyCreateRequest("dependent");
    dependentRequest.depCount = 1;
    dependentRequest.deps[0] = {DeviceDependencyRole::OneWireBus, dependencyResult.deviceId};
    DeviceCreateResult dependentResult = registry.create(dependentRequest, 20);
    TEST_ASSERT_TRUE(dependentResult.ok());

    DeviceMutationResult deleteResult = registry.remove(dependencyResult.deviceId, 30, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_FALSE(deleteResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::InvalidRelationship), static_cast<int>(deleteResult.validation.error));
    TEST_ASSERT_EQUAL_UINT32(1, deleteResult.dependentDeviceIds.size());
    TEST_ASSERT_EQUAL_UINT32(dependentResult.deviceId, deleteResult.dependentDeviceIds[0]);
    TEST_ASSERT_NOT_NULL(registry.runtime(dependencyResult.deviceId));
    TEST_ASSERT_NOT_NULL(registry.runtime(dependentResult.deviceId));
    TEST_ASSERT_NOT_NULL(registry.runtime(dependencyResult.deviceId));
    TEST_ASSERT_NOT_NULL(registry.runtime(dependentResult.deviceId));
    TEST_ASSERT_EQUAL_PTR(registry.runtime(dependencyResult.deviceId),
                          registry.runtime(dependentResult.deviceId)->dependencyRuntime(DeviceDependencyRole::OneWireBus));
    const auto& dependencyDependents = registry.runtime(dependencyResult.deviceId)->dependentRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, dependencyDependents.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(dependentResult.deviceId), dependencyDependents[0]);
}

void test_registry_reassigns_dependency_atomically() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({231, 232, 233});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceTypeDescriptor dependentDescriptor;
    dependentDescriptor.typeId = 59;
    dependentDescriptor.name = "CountingDependent";
    dependentDescriptor.currentConfigVersion = 1;
    dependentDescriptor.dependencyRequirements.push_back({DeviceDependencyRole::OneWireBus, true, {1}});
    dependentDescriptor.createRuntime = &createCountingRuntime;
    TEST_ASSERT_TRUE(types.registerDescriptor(dependentDescriptor));
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult firstDependency = registry.create(makeDummyCreateRequest("dependency-a"), 10);
    TEST_ASSERT_TRUE(firstDependency.ok());
    DeviceCreateResult secondDependency = registry.create(makeDummyCreateRequest("dependency-b"), 20);
    TEST_ASSERT_TRUE(secondDependency.ok());
    registry.tickFastLoop(21);
    registry.tickFastLoop(22);

    DeviceCreateRequest dependentRequest = makeDummyCreateRequest("dependent");
    dependentRequest.typeId = 59;
    dependentRequest.configVersion = 1;
    dependentRequest.depCount = 1;
    dependentRequest.deps[0] = {DeviceDependencyRole::OneWireBus, firstDependency.deviceId};
    DeviceCreateResult dependentResult = registry.create(dependentRequest, 30);
    TEST_ASSERT_TRUE(dependentResult.ok());

    auto* dependentRuntime = dynamic_cast<CountingRuntime*>(registry.runtime(dependentResult.deviceId));
    TEST_ASSERT_NOT_NULL(dependentRuntime);
    TEST_ASSERT_EQUAL_UINT32(1, dependentRuntime->beginCount);
    TEST_ASSERT_EQUAL_UINT32(0, dependentRuntime->reconfigureCount);

    DeviceMutationResult reparentResult =
        registry.setDeps(dependentResult.deviceId, {{DeviceDependencyRole::OneWireBus, secondDependency.deviceId}}, 1, 40,
                         DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(reparentResult.ok());
    TEST_ASSERT_FALSE(reparentResult.pendingPersistence);
    TEST_ASSERT_EQUAL_UINT32(secondDependency.deviceId,
                             registry.runtime(dependentResult.deviceId)->dependencyDeviceId(DeviceDependencyRole::OneWireBus));
    auto* sameDependentRuntime = dynamic_cast<CountingRuntime*>(registry.runtime(dependentResult.deviceId));
    TEST_ASSERT_NOT_NULL(sameDependentRuntime);
    TEST_ASSERT_TRUE(sameDependentRuntime == dependentRuntime);
    TEST_ASSERT_EQUAL_UINT32(1, sameDependentRuntime->beginCount);
    TEST_ASSERT_EQUAL_UINT32(1, sameDependentRuntime->reconfigureCount);
    TEST_ASSERT_EQUAL_PTR(registry.runtime(secondDependency.deviceId),
                          sameDependentRuntime->dependencyRuntime(DeviceDependencyRole::OneWireBus));
    const auto& firstDependencyDependents = registry.runtime(firstDependency.deviceId)->dependentRuntimes();
    TEST_ASSERT_EQUAL_UINT32(0, firstDependencyDependents.size());
    const auto& secondDependencyDependents = registry.runtime(secondDependency.deviceId)->dependentRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, secondDependencyDependents.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(dependentResult.deviceId), secondDependencyDependents[0]);
    TEST_ASSERT_TRUE(registry.remove(firstDependency.deviceId, 50, DevicePersistencePolicy::Immediate).ok());

    DeviceRegistrySnapshot loaded;
    DeviceConfigBlobMap loadedConfigBlobs;
    TEST_ASSERT_TRUE(store.load(loaded, loadedConfigBlobs, &types).ok());
    const auto loadedDependent = std::find_if(
        loaded.records.begin(), loaded.records.end(),
        [dependentId = dependentResult.deviceId](const DeviceRegistryEntry& record) { return record.header.deviceId == dependentId; });
    TEST_ASSERT_TRUE(loadedDependent != loaded.records.end());
    TEST_ASSERT_EQUAL_UINT32(secondDependency.deviceId, loadedDependent->dependencyDeviceId(DeviceDependencyRole::OneWireBus));

    ToggleConfigStorage failingStorage;
    DeviceRegistryStore failingStore(failingStorage);
    TEST_ASSERT_TRUE(failingStore.begin(false));

    FixedDeviceIdSource failingIdSource({241, 242, 243});
    DeviceRegistry failingRegistry(failingStore, types, failingIdSource);
    TEST_ASSERT_TRUE(failingRegistry.begin(0).ok());

    DeviceCreateResult failingDependencyA = failingRegistry.create(makeDummyCreateRequest("dependency-c"), 10);
    TEST_ASSERT_TRUE(failingDependencyA.ok());
    DeviceCreateResult failingDependencyB = failingRegistry.create(makeDummyCreateRequest("dependency-d"), 20);
    TEST_ASSERT_TRUE(failingDependencyB.ok());
    failingRegistry.tickFastLoop(21);
    failingRegistry.tickFastLoop(22);

    DeviceCreateRequest failingDependentRequest = makeDummyCreateRequest("dependent-fail");
    failingDependentRequest.typeId = 59;
    failingDependentRequest.configVersion = 1;
    failingDependentRequest.depCount = 1;
    failingDependentRequest.deps[0] = {DeviceDependencyRole::OneWireBus, failingDependencyA.deviceId};
    DeviceCreateResult failingDependentResult = failingRegistry.create(failingDependentRequest, 30);
    TEST_ASSERT_TRUE(failingDependentResult.ok());

    failingStorage.failNextPutString();
    DeviceMutationResult failingReparent =
        failingRegistry.setDeps(failingDependentResult.deviceId, {{DeviceDependencyRole::OneWireBus, failingDependencyB.deviceId}}, 1, 40,
                                DevicePersistencePolicy::Immediate);
    TEST_ASSERT_FALSE(failingReparent.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::StorageError), static_cast<int>(failingReparent.validation.error));
    TEST_ASSERT_EQUAL_UINT32(
        failingDependencyA.deviceId,
        failingRegistry.runtime(failingDependentResult.deviceId)->dependencyDeviceId(DeviceDependencyRole::OneWireBus));
}

void test_registry_dependency_config_update_reconfigures_dependents() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({251, 252});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceTypeDescriptor dependentDescriptor = makeDependentDescriptor(59, 1);
    dependentDescriptor.createRuntime = &createCountingRuntime;
    TEST_ASSERT_TRUE(types.registerDescriptor(dependentDescriptor));

    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult dependency = registry.create(makeDummyCreateRequest("dependency-update"), 10);
    TEST_ASSERT_TRUE(dependency.ok());

    DeviceCreateRequest dependentRequest = makeDummyCreateRequest("dependent-update");
    dependentRequest.typeId = 59;
    dependentRequest.configVersion = 1;
    dependentRequest.depCount = 1;
    dependentRequest.deps[0] = {DeviceDependencyRole::OneWireBus, dependency.deviceId};
    dependentRequest.persistencePolicy = DevicePersistencePolicy::Delayed;
    DeviceCreateResult dependent = registry.create(dependentRequest, 20);
    TEST_ASSERT_TRUE_MESSAGE(dependent.ok(), dependent.validation.message);

    auto* dependentRuntime = dynamic_cast<CountingRuntime*>(registry.runtime(dependent.deviceId));
    TEST_ASSERT_NOT_NULL(dependentRuntime);
    const uint32_t previousReconfigureCount = dependentRuntime->reconfigureCount;

    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "dependency");
    DeviceMutationResult updated =
        registry.updateConfig(dependency.deviceId, encodeDummyConfig(config), DummyDevice::descriptor().currentConfigVersion, 30,
                              DevicePersistencePolicy::Delayed);
    TEST_ASSERT_TRUE_MESSAGE(updated.ok(), updated.validation.message);

    auto* sameDependentRuntime = dynamic_cast<CountingRuntime*>(registry.runtime(dependent.deviceId));
    TEST_ASSERT_NOT_NULL(sameDependentRuntime);
    TEST_ASSERT_TRUE(sameDependentRuntime == dependentRuntime);
    TEST_ASSERT_EQUAL_UINT32(previousReconfigureCount + 1U, sameDependentRuntime->reconfigureCount);
    TEST_ASSERT_EQUAL_PTR(registry.runtime(dependency.deviceId), sameDependentRuntime->dependencyRuntime(DeviceDependencyRole::OneWireBus));
}

void test_registry_propagates_dependency_status_and_recovers() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({221, 222});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_TRUE(types.registerDescriptor(makeCommandableDescriptor(58)));
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateRequest dependencyRequest = makeDummyCreateRequest("dependency");
    dependencyRequest.typeId = 58;
    DeviceCreateResult dependencyResult = registry.create(dependencyRequest, 10);
    TEST_ASSERT_TRUE(dependencyResult.ok());

    DeviceCreateRequest dependentRequest = makeDummyCreateRequest("dependent");
    dependentRequest.depCount = 1;
    dependentRequest.deps[0] = {DeviceDependencyRole::OneWireBus, dependencyResult.deviceId};
    DeviceCreateResult dependentResult = registry.create(dependentRequest, 20);
    TEST_ASSERT_TRUE(dependentResult.ok());
    TEST_ASSERT_NOT_NULL(registry.runtime(dependentResult.deviceId));

    DeviceMutationResult disableResult = registry.setEnabled(dependencyResult.deviceId, false, 30, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(disableResult.ok());
    TEST_ASSERT_NOT_NULL(registry.runtime(dependentResult.deviceId));
    auto disabledRecords = registry.list();
    const auto disabledDependent = std::find_if(
        disabledRecords.begin(), disabledRecords.end(),
        [dependentId = dependentResult.deviceId](const DeviceRegistryEntry& record) { return record.header.deviceId == dependentId; });
    TEST_ASSERT_TRUE(disabledDependent != disabledRecords.end());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Disabled), static_cast<int>(disabledDependent->status));
    TEST_ASSERT_EQUAL_PTR(registry.runtime(dependencyResult.deviceId),
                          registry.runtime(dependentResult.deviceId)->dependencyRuntime(DeviceDependencyRole::OneWireBus));
    const auto& blockedDependents = registry.runtime(dependencyResult.deviceId)->dependentRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, blockedDependents.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(dependentResult.deviceId), blockedDependents[0]);

    DeviceMutationResult enableResult = registry.setEnabled(dependencyResult.deviceId, true, 40, DevicePersistencePolicy::Immediate);
    TEST_ASSERT_TRUE(enableResult.ok());
    registry.tickFastLoop(41);
    registry.tickFastLoop(42);
    registry.tickFastLoop(43);
    registry.tickFastLoop(44);
    TEST_ASSERT_NOT_NULL(registry.runtime(dependentResult.deviceId));
    auto recoveredRecords = registry.list();
    const auto recoveredDependent = std::find_if(
        recoveredRecords.begin(), recoveredRecords.end(),
        [dependentId = dependentResult.deviceId](const DeviceRegistryEntry& record) { return record.header.deviceId == dependentId; });
    TEST_ASSERT_TRUE(recoveredDependent != recoveredRecords.end());
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(recoveredDependent->status));
    const auto& recoveredDependents = registry.runtime(dependencyResult.deviceId)->dependentRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, recoveredDependents.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(dependentResult.deviceId), recoveredDependents[0]);

    DeviceMutationResult faultResult = registry.command(
        DeviceCommand{DeviceCommandType::SetStatus, dependencyResult.deviceId, "fault", DevicePersistencePolicy::Delayed}, 50);
    TEST_ASSERT_TRUE(faultResult.ok());
    registry.tickFastLoop(51);
    registry.tickFastLoop(52);
    TEST_ASSERT_NOT_NULL(registry.runtime(dependentResult.deviceId));
    auto faultedRecords = registry.list();
    const auto faultedDependent = std::find_if(
        faultedRecords.begin(), faultedRecords.end(),
        [dependentId = dependentResult.deviceId](const DeviceRegistryEntry& record) { return record.header.deviceId == dependentId; });
    TEST_ASSERT_TRUE(faultedDependent != faultedRecords.end());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::DependencyBlocked), static_cast<int>(faultedDependent->status));
}

void test_registry_backfills_dependency_links_after_begin() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({261, 262});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_TRUE(types.registerDescriptor(makeCommandableDescriptor(58)));
    DeviceTypeDescriptor dependentDescriptor = makeDependentDescriptor(59, 58);
    dependentDescriptor.createRuntime = &createCountingRuntime;
    TEST_ASSERT_TRUE(types.registerDescriptor(dependentDescriptor));

    DeviceRegistrySnapshot snapshot{};
    DeviceConfigBlobMap configBlobs{};

    DummyDeviceConfigV1 dependentConfig{};
    dependentConfig.enabled = true;
    std::snprintf(dependentConfig.name, sizeof(dependentConfig.name), "%s", "dependent");
    const DeviceConfigBlob dependentBlob = encodeDummyConfig(dependentConfig);

    DeviceRegistryEntry dependentRecord{};
    dependentRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    dependentRecord.header.deviceId = 261;
    dependentRecord.header.typeId = 59;
    dependentRecord.header.configVersion = 1U;
    dependentRecord.header.configRevision = 1U;
    dependentRecord.header.payloadLength = static_cast<uint32_t>(dependentBlob.size());
    dependentRecord.depCount = 1U;
    dependentRecord.deps[0] = {DeviceDependencyRole::OneWireBus, 262};
    dependentRecord.status = DeviceStatus::Ready;
    snapshot.indexEntries.push_back({261, 59});
    snapshot.records.push_back(dependentRecord);
    configBlobs[261] = dependentBlob;

    DummyDeviceConfigV1 dependencyConfig{};
    dependencyConfig.enabled = true;
    std::snprintf(dependencyConfig.name, sizeof(dependencyConfig.name), "%s", "dependency");
    const DeviceConfigBlob dependencyBlob = encodeDummyConfig(dependencyConfig);

    DeviceRegistryEntry dependencyRecord{};
    dependencyRecord.header.recordVersion = kDeviceRecordHeaderVersion;
    dependencyRecord.header.deviceId = 262;
    dependencyRecord.header.typeId = 58;
    dependencyRecord.header.configVersion = 1U;
    dependencyRecord.header.configRevision = 1U;
    dependencyRecord.header.payloadLength = static_cast<uint32_t>(dependencyBlob.size());
    dependencyRecord.status = DeviceStatus::Ready;
    snapshot.indexEntries.push_back({262, 58});
    snapshot.records.push_back(dependencyRecord);
    configBlobs[262] = dependencyBlob;

    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());

    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    auto* dependentRuntime = dynamic_cast<CountingRuntime*>(registry.runtime(261));
    TEST_ASSERT_NOT_NULL(dependentRuntime);
    TEST_ASSERT_EQUAL_PTR(registry.runtime(262), dependentRuntime->dependencyRuntime(DeviceDependencyRole::OneWireBus));
    const auto& dependencyDependents = registry.runtime(262)->dependentRuntimes();
    TEST_ASSERT_EQUAL_UINT32(1, dependencyDependents.size());
    TEST_ASSERT_EQUAL_PTR(registry.runtime(261), dependencyDependents[0]);
}

void test_registry_set_deps_command_normalization() {
    MemoryConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({311, 312, 313});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult dependencyA = registry.create(makeDummyCreateRequest("dependency-a"), 10);
    DeviceCreateResult dependencyB = registry.create(makeDummyCreateRequest("dependency-b"), 20);
    TEST_ASSERT_TRUE(dependencyA.ok());
    TEST_ASSERT_TRUE(dependencyB.ok());

    DeviceCreateRequest dependentRequest = makeDummyCreateRequest("dependent");
    dependentRequest.depCount = 1;
    dependentRequest.deps[0] = {DeviceDependencyRole::OneWireBus, dependencyA.deviceId};
    DeviceCreateResult dependent = registry.create(dependentRequest, 30);
    TEST_ASSERT_TRUE(dependent.ok());

    DeviceMutationResult commandResult =
        registry.command(DeviceCommand{DeviceCommandType::SetDeps, dependent.deviceId,
                                       DeviceCommand::DepsPayload{{{DeviceDependencyRole::OneWireBus, dependencyB.deviceId}}, 1},
                                       DevicePersistencePolicy::Immediate},
                         40);
    TEST_ASSERT_TRUE(commandResult.ok());
    TEST_ASSERT_EQUAL_UINT32(dependencyB.deviceId,
                             registry.runtime(dependent.deviceId)->dependencyDeviceId(DeviceDependencyRole::OneWireBus));
    TEST_ASSERT_EQUAL_PTR(registry.runtime(dependencyB.deviceId),
                          registry.runtime(dependent.deviceId)->dependencyRuntime(DeviceDependencyRole::OneWireBus));

    DeviceMutationResult clearDepsResult = registry.command(
        DeviceCommand{DeviceCommandType::SetDeps, dependent.deviceId, DeviceCommand::DepsPayload{}, DevicePersistencePolicy::Immediate},
        41);
    TEST_ASSERT_TRUE(clearDepsResult.ok());
    TEST_ASSERT_FALSE(registry.runtime(dependent.deviceId)->hasDependencies());
    TEST_ASSERT_NULL(registry.runtime(dependent.deviceId)->dependencyRuntime(DeviceDependencyRole::OneWireBus));

    DeviceCommand invalidPayload{};
    invalidPayload.type = DeviceCommandType::SetDeps;
    invalidPayload.deviceId = dependent.deviceId;
    invalidPayload.persistencePolicy = DevicePersistencePolicy::Immediate;
    DeviceMutationResult invalidPayloadResult = registry.command(invalidPayload, 42);
    TEST_ASSERT_FALSE(invalidPayloadResult.ok());
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceError::BoundsExceeded), static_cast<int>(invalidPayloadResult.validation.error));
}

void test_registry_emits_required_event_kinds() {
    MemoryConfigStorage storage;
    DeviceRegistryStore registryStore(storage);
    RetainedStateStore retainedStore(storage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));

    DeviceEventDispatcher dispatcher;
    RecordingSink sink{};
    TEST_ASSERT_TRUE(dispatcher.registerSink(sink));

    FixedDeviceIdSource idSource({321, 322, 323});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_TRUE(types.registerDescriptor(makeRetainedDescriptor(57)));
    DeviceRegistry registry(registryStore, types, idSource, &retainedStore, &dispatcher);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateResult created = registry.command(makeDummyCreateRequest("events"), 10);
    TEST_ASSERT_TRUE(created.ok());
    dispatcher.tickFastLoop(11);

    DeviceMutationResult renamed =
        registry.command(DeviceCommand{DeviceCommandType::Rename, created.deviceId, "events-2", DevicePersistencePolicy::Delayed}, 20);
    TEST_ASSERT_TRUE(renamed.ok());
    dispatcher.tickFastLoop(21);

    DeviceMutationResult rejected =
        registry.command(DeviceCommand{DeviceCommandType::SetStatus, 9999, "fault", DevicePersistencePolicy::Immediate}, 22);
    TEST_ASSERT_FALSE(rejected.ok());
    dispatcher.tickFastLoop(23);

    DeviceCreateRequest retainedRequest = makeDummyCreateRequest("retained-events");
    retainedRequest.typeId = 57;
    retainedRequest.persistencePolicy = DevicePersistencePolicy::Delayed;
    DeviceCreateResult retainedCreated = registry.command(retainedRequest, 24);
    TEST_ASSERT_TRUE(retainedCreated.ok());
    dispatcher.tickFastLoop(25);

    DeviceMutationResult retained = registry.setRetainedState(retainedCreated.deviceId, "output=1", 26, DevicePersistencePolicy::Coalesced);
    TEST_ASSERT_TRUE(retained.ok());
    dispatcher.tickFastLoop(27);

    TEST_ASSERT_TRUE(registry.flushNow().ok());
    dispatcher.tickFastLoop(28);

    DeviceMutationResult deleted =
        registry.command(DeviceCommand{DeviceCommandType::Delete, created.deviceId, "", DevicePersistencePolicy::Immediate}, 29);
    TEST_ASSERT_TRUE(deleted.ok());
    dispatcher.tickFastLoop(30);

    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::DeviceCreated));
    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::DeviceUpdated));
    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::DeviceDeleted));
    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::StatusChanged));
    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::RetainedStateChanged));
    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::CommandAccepted));
    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::CommandRejected));
    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::ConfigPersisted));
    TEST_ASSERT_TRUE(hasEventKind(sink.events, DeviceEventKind::PersistencePendingCleared));
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
    DeviceConfigBlobMap loadedConfigBlobs;
    TEST_ASSERT_TRUE(store.load(loaded, loadedConfigBlobs, &types).ok());
    assertDummyConfigName(loadedConfigBlobs, 401, "tick-dummy-2");
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

    DeviceCreateRequest request = makeDummyCreateRequest("counting");
    request.typeId = 55;
    request.configVersion = 1;
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
    TEST_ASSERT_NULL(registry.runtime(601));
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
    TEST_ASSERT_FALSE(registry.dirtyIndex());
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyConfigRecordIds().size());
    TEST_ASSERT_EQUAL_UINT32(20, registry.firstDirtyAt());
    TEST_ASSERT_EQUAL_UINT32(20, registry.lastChangeAt());

    storage.failNextPutString();
    DeviceValidationResult flushResult = registry.flushNow();
    TEST_ASSERT_FALSE(flushResult.ok());
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());
    TEST_ASSERT_FALSE(registry.dirtyIndex());
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyConfigRecordIds().size());

    TEST_ASSERT_TRUE(registry.flushNow().ok());
    TEST_ASSERT_FALSE(registry.hasPendingPersistence());

    DeviceRegistrySnapshot loaded;
    DeviceConfigBlobMap loadedConfigBlobs;
    TEST_ASSERT_TRUE(store.load(loaded, loadedConfigBlobs, &types).ok());
    assertDummyConfigName(loadedConfigBlobs, 701, "dirty-dummy-2");
}

void test_registry_index_persistence_uses_bounded_blob_not_string() {
    RecordingConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    DeviceRegistrySnapshot snapshot{};
    DeviceConfigBlobMap configBlobs{};
    DummyDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "bounded");
    const DeviceConfigBlob configBlob = encodeDummyConfig(config);

    for (uint32_t index = 1; index <= 3; ++index) {
        DeviceRegistryEntry record{};
        record.header.recordVersion = kDeviceRecordHeaderVersion;
        record.header.deviceId = index;
        record.header.typeId = 1;
        record.header.configVersion = DummyDevice::descriptor().currentConfigVersion;
        record.header.configRevision = 1;
        record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
        record.status = DeviceStatus::Ready;
        snapshot.indexEntries.push_back({index, 1});
        snapshot.records.push_back(record);
        configBlobs[index] = configBlob;
    }

    TEST_ASSERT_TRUE(store.save(snapshot, configBlobs).ok());
    TEST_ASSERT_EQUAL_UINT32(0, storage.putStringKeys.size());
    TEST_ASSERT_EQUAL_UINT32(1, storage.countBlobWrites("index"));
    TEST_ASSERT_TRUE(storage.putBlobSizes["index"] <= kMaxRegistryIndexBytes);
}

void test_registry_failed_index_commit_keeps_previous_registry_recoverable() {
    RecordingConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({901, 902});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("stable"), 10).ok());

    DeviceCreateRequest delayed = makeDummyCreateRequest("orphan");
    delayed.persistencePolicy = DevicePersistencePolicy::Delayed;
    DeviceCreateResult created = registry.create(delayed, 20);
    TEST_ASSERT_TRUE(created.ok());
    storage.clearLog();
    storage.failNextWriteFor("index");

    DeviceValidationResult flushResult = registry.flushNow();
    TEST_ASSERT_FALSE(flushResult.ok());
    TEST_ASSERT_TRUE(registry.hasPendingPersistence());

    char orphanKey[32]{};
    makeRecordKey(created.deviceId, orphanKey);
    TEST_ASSERT_EQUAL_UINT32(1, storage.countBlobWrites(orphanKey));
    TEST_ASSERT_EQUAL_UINT32(1, storage.countBlobWrites("index"));
    TEST_ASSERT_EQUAL_UINT32(0, storage.countUIntWrites("version"));

    DeviceRegistrySnapshot loaded{};
    DeviceConfigBlobMap loadedConfigBlobs{};
    TEST_ASSERT_TRUE(store.load(loaded, loadedConfigBlobs, &types).ok());
    TEST_ASSERT_EQUAL_UINT32(1, loaded.records.size());
    TEST_ASSERT_EQUAL_UINT32(901, loaded.records[0].header.deviceId);
    assertDummyConfigName(loadedConfigBlobs, 901, "stable");
}

void test_registry_load_resets_unsupported_registry_format_without_crashing() {
    RecordingConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));
    TEST_ASSERT_TRUE(storage.putUInt("version", 2));
    TEST_ASSERT_TRUE(storage.putBlob("index", reinterpret_cast<const uint8_t*>("old"), 3));
    storage.clearLog();

    DeviceRegistrySnapshot loaded{};
    DeviceConfigBlobMap loadedConfigBlobs{};
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_TRUE(store.load(loaded, loadedConfigBlobs, &types).ok());
    TEST_ASSERT_TRUE(loaded.records.empty());
    TEST_ASSERT_TRUE(loaded.indexEntries.empty());
    TEST_ASSERT_TRUE(loadedConfigBlobs.empty());
    TEST_ASSERT_EQUAL_UINT32(1, storage.clearCount);
}

void test_registry_selective_flush_writes_only_dirty_record_and_not_index() {
    RecordingConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({911, 912});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("first"), 10).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("second"), 20).ok());

    TEST_ASSERT_TRUE(registry.rename(911, "first-renamed", 30, DevicePersistencePolicy::Delayed).ok());
    storage.clearLog();
    TEST_ASSERT_TRUE(registry.flushNow().ok());

    char firstKey[32]{};
    char secondKey[32]{};
    makeRecordKey(911, firstKey);
    makeRecordKey(912, secondKey);
    TEST_ASSERT_EQUAL_UINT32(1, storage.countBlobWrites(firstKey));
    TEST_ASSERT_EQUAL_UINT32(0, storage.countBlobWrites(secondKey));
    TEST_ASSERT_EQUAL_UINT32(0, storage.countBlobWrites("index"));
    TEST_ASSERT_EQUAL_UINT32(0, storage.countUIntWrites("version"));
}

void test_registry_delayed_rename_does_not_dirty_or_rewrite_index() {
    RecordingConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({921});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("rename-source"), 10).ok());

    TEST_ASSERT_TRUE(registry.rename(921, "rename-target", 20, DevicePersistencePolicy::Delayed).ok());
    TEST_ASSERT_FALSE(registry.dirtyIndex());
    TEST_ASSERT_EQUAL_UINT32(1, registry.dirtyConfigRecordIds().size());
    TEST_ASSERT_EQUAL_UINT32(921, registry.dirtyConfigRecordIds()[0]);

    storage.clearLog();
    TEST_ASSERT_TRUE(registry.flushNow().ok());
    TEST_ASSERT_EQUAL_UINT32(0, storage.countBlobWrites("index"));
    TEST_ASSERT_EQUAL_UINT32(0, storage.countUIntWrites("version"));
}

void test_registry_create_orphan_and_delete_cleanup_ordering() {
    RecordingConfigStorage storage;
    DeviceRegistryStore store(storage);
    TEST_ASSERT_TRUE(store.begin(false));

    FixedDeviceIdSource idSource({931, 932});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(store, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(0).ok());
    TEST_ASSERT_TRUE(registry.create(makeDummyCreateRequest("keep"), 10).ok());

    DeviceCreateRequest orphanRequest = makeDummyCreateRequest("orphan");
    orphanRequest.persistencePolicy = DevicePersistencePolicy::Delayed;
    DeviceCreateResult orphanCreate = registry.create(orphanRequest, 20);
    TEST_ASSERT_TRUE(orphanCreate.ok());
    storage.failNextWriteFor("index");
    TEST_ASSERT_FALSE(registry.flushNow().ok());

    DeviceRegistrySnapshot loadedAfterFailedCreate{};
    DeviceConfigBlobMap blobsAfterFailedCreate{};
    TEST_ASSERT_TRUE(store.load(loadedAfterFailedCreate, blobsAfterFailedCreate, &types).ok());
    TEST_ASSERT_EQUAL_UINT32(1, loadedAfterFailedCreate.records.size());
    TEST_ASSERT_EQUAL_UINT32(931, loadedAfterFailedCreate.records[0].header.deviceId);

    TEST_ASSERT_TRUE(registry.flushNow().ok());
    TEST_ASSERT_TRUE(registry.remove(orphanCreate.deviceId, 30, DevicePersistencePolicy::Delayed).ok());
    storage.clearLog();
    TEST_ASSERT_TRUE(registry.flushNow().ok());

    char removedKey[32]{};
    makeRecordKey(orphanCreate.deviceId, removedKey);
    TEST_ASSERT_EQUAL_UINT32(1, storage.countBlobWrites("index"));
    TEST_ASSERT_EQUAL_UINT32(1, storage.countUIntWrites("version"));
    TEST_ASSERT_TRUE(storage.removed(removedKey));

    DeviceRegistrySnapshot loadedAfterDelete{};
    DeviceConfigBlobMap blobsAfterDelete{};
    TEST_ASSERT_TRUE(store.load(loadedAfterDelete, blobsAfterDelete, &types).ok());
    TEST_ASSERT_EQUAL_UINT32(1, loadedAfterDelete.records.size());
    TEST_ASSERT_EQUAL_UINT32(931, loadedAfterDelete.records[0].header.deviceId);
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
    DeviceConfigBlobMap loadedConfigBlobs;
    TEST_ASSERT_TRUE(store.load(loaded, loadedConfigBlobs, &types).ok());
    assertDummyConfigName(loadedConfigBlobs, 801, "delay-dummy-3");
}

void test_registry_coalesces_retained_state_updates() {
    MemoryConfigStorage storage;
    DeviceRegistryStore registryStore(storage);
    RetainedStateStore retainedStore(storage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    TEST_ASSERT_TRUE(retainedStore.begin(false));
    FixedDeviceIdSource idSource({501, 502});
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    TEST_ASSERT_TRUE(types.registerDescriptor(makeRetainedDescriptor(57)));
    DeviceRegistry registry(registryStore, types, idSource, &retainedStore);
    TEST_ASSERT_TRUE(registry.begin(0).ok());

    DeviceCreateRequest retainedRequest = makeDummyCreateRequest("retained-dummy");
    retainedRequest.typeId = 57;
    TEST_ASSERT_TRUE(registry.create(retainedRequest, 10).ok());

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
    RUN_TEST(test_registry_rejects_dependency_delete_with_dependents);
    RUN_TEST(test_registry_reassigns_dependency_atomically);
    RUN_TEST(test_registry_propagates_dependency_status_and_recovers);
    RUN_TEST(test_registry_backfills_dependency_links_after_begin);
    RUN_TEST(test_registry_set_deps_command_normalization);
    RUN_TEST(test_registry_dependency_config_update_reconfigures_dependents);
    RUN_TEST(test_registry_emits_required_event_kinds);
    RUN_TEST(test_registry_invokes_only_declared_cadences);
    RUN_TEST(test_registry_immediate_persistence_failure_rolls_back_create);
    RUN_TEST(test_registry_delayed_dirty_state_and_forced_flush);
    RUN_TEST(test_registry_index_persistence_uses_bounded_blob_not_string);
    RUN_TEST(test_registry_failed_index_commit_keeps_previous_registry_recoverable);
    RUN_TEST(test_registry_load_resets_unsupported_registry_format_without_crashing);
    RUN_TEST(test_registry_selective_flush_writes_only_dirty_record_and_not_index);
    RUN_TEST(test_registry_delayed_rename_does_not_dirty_or_rewrite_index);
    RUN_TEST(test_registry_create_orphan_and_delete_cleanup_ordering);
    RUN_TEST(test_registry_max_delay_flushes_after_repeated_dirty_updates);
    RUN_TEST(test_registry_coalesces_retained_state_updates);
    return UNITY_END();
}
