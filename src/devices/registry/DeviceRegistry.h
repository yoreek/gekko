#pragma once

#include "devices/core/DeviceBaseConfig.h"
#include "devices/core/DeviceIdGenerator.h"
#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistryEventReporter.h"
#include "devices/registry/DeviceRegistryMutex.h"
#include "devices/registry/DeviceRegistryPersistenceCoordinator.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceRetainedDataStore.h"
#include "devices/registry/DeviceRuntimeStore.h"

#include <map>
#include <memory>
#include <vector>

namespace ewfm {

class DeviceEventDispatcher;
class DeviceScopedDataStore;

struct DeviceCreateRequest {
    DeviceTypeId typeId{0};
    DeviceBaseConfigV1 baseConfig{};
    BoundedBlob<kMaxDeviceConfigBytes> configBlob{};
    uint32_t configVersion{0};
    std::array<DeviceDependencyLink, kMaxDeviceDependencies> deps{};
    uint8_t depCount{0};

    bool assignName(const char* name) {
        if (name == nullptr) {
            return false;
        }
        return copyBoundedText(baseConfig.name, name);
    }

    bool assignName(const std::string& name) {
        return assignName(name.c_str());
    }

    void setEnabled(bool enabled) {
        baseConfig.enabled = enabled ? 1U : 0U;
    }

    bool isEnabled() const {
        return baseConfig.enabled != 0U;
    }

    bool hasDeps() const {
        return dependencyCount() > 0;
    }

    uint8_t dependencyCount() const {
        return depCount;
    }

    const DeviceDependencyLink* dependencyLinks() const {
        return deps.data();
    }
};

struct DeviceMutationResult {
    DeviceValidationResult validation{};
    bool pendingPersistence{false};
    std::vector<DeviceId> dependentDeviceIds{};
#ifdef UNIT_TEST
#endif

    bool ok() const {
        return validation.ok();
    }
};

struct DeviceCreateResult {
    DeviceValidationResult validation{};
    DeviceId deviceId{0};
    bool pendingPersistence{false};

    bool ok() const {
        return validation.ok();
    }
};

struct DevicePersistedStateUpdate {
    DeviceId deviceId{0};
    const uint8_t* data{nullptr};
    size_t size{0U};
};

class DeviceRegistry {
public:
    // Defaults match the previous hardcoded constants; App overrides these from ConfigStore's
    // PersistenceConfig at boot (and on every settings PUT) via setPersistenceDelays().
    static constexpr uint32_t kDefaultPersistenceDebounceMs = 500;
    static constexpr uint32_t kDefaultPersistenceMaxDelayMs = 2000;

    DeviceRegistry(DeviceRegistryStore& store, const DeviceTypeRegistry& typeRegistry, IDeviceIdSource& idSource,
                   DeviceRetainedDataStore* retainedStateStore = nullptr, DeviceScopedDataStore* persistedStateStore = nullptr,
                   DeviceEventDispatcher* eventDispatcher = nullptr);
    DeviceRegistry(DeviceRegistryStore& store, const DeviceTypeRegistry& typeRegistry, IDeviceIdSource& idSource,
                   DeviceRetainedDataStore* retainedStateStore, DeviceEventDispatcher* eventDispatcher);

    // debounceMs: how long after the *last* change to wait before flushing (quiet period).
    // maxDelayMs: hard cap measured from the *first* unflushed change, regardless of continued
    // activity -- the real ceiling on flash write frequency under continuous churn.
    void setPersistenceDelays(uint32_t debounceMs, uint32_t maxDelayMs);

    DeviceValidationResult begin(uint32_t now = 0);
    void tick(uint32_t now);
    void tickFastLoop(uint32_t now);
    void tick100ms(uint32_t now);
    void tick1s(uint32_t now);

    uint32_t registryRevision() const;
    bool hasPendingPersistence() const;

    const DeviceId* pinOwners() const {
        return pinOwner_;
    }

    std::vector<DeviceRegistryEntry> list() const;
    template <typename Fn> void forEachRuntime(Fn&& visitor) const {
        DeviceRegistryLockGuard guard(mutex_);
        for (const auto& entry : runtimes_) {
            if (entry.second.runtime != nullptr) {
                visitor(*entry.second.runtime);
            }
        }
    }
    DeviceStatus effectiveStatus(DeviceId deviceId) const;
    IDeviceRuntime* runtime(DeviceId deviceId);
    const IDeviceRuntime* runtime(DeviceId deviceId) const;

    DeviceCreateResult create(const DeviceCreateRequest& request, uint32_t now);
    DeviceCreateResult command(const DeviceCreateRequest& request, uint32_t now);
    DeviceMutationResult updateConfig(DeviceId deviceId, const BoundedBlob<kMaxDeviceConfigBytes>& configBlob, uint32_t configVersion,
                                      uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed);
    DeviceMutationResult updateConfigAndDeps(DeviceId deviceId, const BoundedBlob<kMaxDeviceConfigBytes>& configBlob,
                                             uint32_t configVersion, const DeviceBaseConfigV1& baseConfig, bool depsProvided,
                                             const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                             uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed,
                                             const uint8_t* persistedStateData = nullptr, size_t persistedStateSize = 0U);
    DeviceMutationResult setDeps(DeviceId deviceId, const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                 uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Immediate);
    DeviceMutationResult remove(DeviceId deviceId, uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Immediate);
    DeviceMutationResult command(const DeviceCommand& command, uint32_t now);
    DeviceValidationResult flushNow();
    DeviceValidationResult restore(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs,
                                   uint32_t registryRevision, uint32_t now);
    DeviceValidationResult restore(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs,
                                   const std::vector<DevicePersistedStateUpdate>& persistedStateUpdates, uint32_t registryRevision,
                                   uint32_t now);
    DeviceValidationResult applyPersistedStateUpdate(DeviceId deviceId, const uint8_t* data, size_t size, uint32_t now);

    bool dirtyIndex() const;
    std::vector<DeviceId> dirtyConfigRecordIds() const;
    std::vector<DeviceId> dirtyRetainedStateIds() const;
    uint32_t firstDirtyAt() const;
    uint32_t lastChangeAt() const;

private:
    DeviceValidationResult validateSnapshot(const DeviceRegistrySnapshot& snapshot) const;
    DeviceValidationResult validateSnapshot(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs) const;
    DeviceValidationResult validateRecord(const DeviceRegistryEntry& record, const DeviceTypeDescriptor& descriptor,
                                          const DeviceConfigBlob& configBlob) const;
    DeviceValidationResult validateDependencies(const DeviceRegistrySnapshot& snapshot, const DeviceRegistryEntry& record) const;
    DeviceValidationResult validateAcyclicDependencyGraph(const DeviceRegistrySnapshot& snapshot) const;
    void discardInvalidLoadRelationships(DeviceRegistrySnapshot& snapshot, DeviceConfigBlobMap& configBlobs,
                                         std::vector<DeviceId>& discardedDeviceIds) const;
    std::vector<DeviceId> dependentDeviceIds(DeviceId deviceId) const;
    void syncRuntimeDependencyLinks(DeviceId deviceId);
    DeviceStatus effectiveStatusForRuntime(const IDeviceRuntime& runtime) const;
    void refreshDependentRuntimeStates(uint32_t now);
    void captureDirtyRuntimeRetainedStates(uint32_t now);
    DeviceValidationResult persistIfNeeded(DevicePersistencePolicy policy);
    DeviceValidationResult reloadRuntimeFor(DeviceRuntimeMap& target, const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob,
                                            bool loadPersistedState = true);
    DeviceValidationResult reloadRuntimeFor(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    DeviceValidationResult replaceRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    DeviceValidationResult buildSnapshot(DeviceRegistrySnapshot& snapshot, DeviceConfigBlobMap& configBlobs) const;
    void clearRuntime(DeviceId deviceId);
    void emitRuntimeStatusChanges();
    DeviceValidationResult captureRuntimeRetainedState(DeviceId deviceId, uint32_t now);
    IDevicePersistedState* persistedStateRuntime(DeviceId deviceId);
    const IDevicePersistedState* persistedStateRuntime(DeviceId deviceId) const;
    DeviceValidationResult restoreImpl(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs,
                                       const std::vector<DevicePersistedStateUpdate>* persistedStateUpdates, uint32_t registryRevision,
                                       uint32_t now);

    DeviceRegistryStore& store_;
    const DeviceTypeRegistry& typeRegistry_;
    IDeviceIdSource& idSource_;
    DeviceRetainedDataStore* retainedStateStore_{nullptr};
    DeviceScopedDataStore* persistedStateStore_{nullptr};
    DeviceRegistryEventReporter eventReporter_{};
    DeviceRegistryPersistenceCoordinator persistence_{};
    uint32_t persistenceDebounceMs_{kDefaultPersistenceDebounceMs};
    uint32_t persistenceMaxDelayMs_{kDefaultPersistenceMaxDelayMs};
    DeviceRuntimeMap runtimes_{};
    uint32_t registryRevision_{0};
    mutable DeviceRegistryMutex mutex_{};
    // 0 = free, else the owning DeviceId. Indexed directly by raw GPIO number, rebuilt from
    // scratch by reloadRuntimeFor() on every device load (including controller boot), kept in
    // sync incrementally on config update and removal. See docs/gpio-pin-occupancy.md.
    DeviceId pinOwner_[kGpioPinTableSize]{};
};

} // namespace ewfm
