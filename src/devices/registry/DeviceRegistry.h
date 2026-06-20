#pragma once

#include "devices/core/DeviceIdGenerator.h"
#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistryEventReporter.h"
#include "devices/registry/DeviceRegistryPersistenceCoordinator.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/DeviceRuntimeStore.h"
#include "devices/registry/RetainedStateStore.h"

#include <map>
#include <memory>
#include <vector>

namespace ewfm {

class DeviceEventDispatcher;

struct DeviceCreateRequest {
    DeviceTypeId typeId{0};
    std::string name{};
    BoundedBlob<kMaxDeviceConfigBytes> configBlob{};
    uint32_t configVersion{0};
    bool enabled{true};
    std::array<DeviceDependencyLink, kMaxDeviceDependencies> deps{};
    uint8_t depCount{0};
#ifdef UNIT_TEST
    bool hasParent{false};
    DeviceId parentDeviceId{0};
#endif
    DevicePersistencePolicy persistencePolicy{DevicePersistencePolicy::Immediate};

    bool hasDeps() const {
        return dependencyCount() > 0;
    }

    uint8_t dependencyCount() const {
#ifdef UNIT_TEST
        if (depCount == 0 && hasParent && parentDeviceId != 0U) {
            return 1;
        }
#endif
        return depCount;
    }

    const DeviceDependencyLink* dependencyLinks() const {
#ifdef UNIT_TEST
        if (depCount == 0 && hasParent && parentDeviceId != 0U) {
            static thread_local DeviceDependencyLink legacyLinks[1];
            legacyLinks[0] = {DeviceDependencyRole::OneWireBus, parentDeviceId};
            return legacyLinks;
        }
#endif
        return deps.data();
    }
};

struct DeviceMutationResult {
    DeviceValidationResult validation{};
    bool pendingPersistence{false};
    std::vector<DeviceId> dependentDeviceIds{};
#ifdef UNIT_TEST
    std::vector<DeviceId> dependentChildDeviceIds{};
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

class DeviceRegistry {
public:
    static constexpr uint32_t kPersistenceDebounceMs = 500;
    static constexpr uint32_t kPersistenceMaxDelayMs = 2000;

    DeviceRegistry(DeviceRegistryStore& store, const DeviceTypeRegistry& typeRegistry, IDeviceIdSource& idSource,
                   RetainedStateStore* retainedStateStore = nullptr, DeviceEventDispatcher* eventDispatcher = nullptr);

    DeviceValidationResult begin(uint32_t now = 0);
    void tick(uint32_t now);
    void tickFastLoop(uint32_t now);
    void tick100ms(uint32_t now);
    void tick1s(uint32_t now);

    uint32_t registryRevision() const;
    bool hasPendingPersistence() const;

    std::vector<DeviceRegistryEntry> list() const;
    template <typename Fn> void forEachRuntime(Fn&& visitor) const {
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
    DeviceMutationResult rename(DeviceId deviceId, const std::string& name, uint32_t now,
                                DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed);
    DeviceMutationResult updateConfig(DeviceId deviceId, const BoundedBlob<kMaxDeviceConfigBytes>& configBlob, uint32_t configVersion,
                                      uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed);
    DeviceMutationResult updateConfigAndDeps(DeviceId deviceId, const BoundedBlob<kMaxDeviceConfigBytes>& configBlob,
                                             uint32_t configVersion, bool depsProvided,
                                             const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                             uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed);
    DeviceMutationResult setDeps(DeviceId deviceId, const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps, uint8_t depCount,
                                 uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Immediate);
#ifdef UNIT_TEST
    DeviceMutationResult setParent(DeviceId deviceId, bool hasParent, DeviceId parentDeviceId, uint32_t now,
                                   DevicePersistencePolicy policy = DevicePersistencePolicy::Immediate);
#endif
    DeviceMutationResult setEnabled(DeviceId deviceId, bool enabled, uint32_t now,
                                    DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed);
    DeviceMutationResult remove(DeviceId deviceId, uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Immediate);
    DeviceMutationResult setRetainedState(DeviceId deviceId, const std::string& payload, uint32_t now,
                                          DevicePersistencePolicy policy = DevicePersistencePolicy::Coalesced);
    DeviceMutationResult command(const DeviceCommand& command, uint32_t now);
    DeviceValidationResult flushNow();

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
    std::vector<DeviceId> dependentDeviceIds(DeviceId deviceId) const;
    void syncRuntimeDependencyLinks(DeviceId deviceId);
    DeviceStatus effectiveStatusForRuntime(const IDeviceRuntime& runtime) const;
    void refreshDependentRuntimeStates(uint32_t now);
    DeviceValidationResult persistIfNeeded(DevicePersistencePolicy policy);
    DeviceValidationResult reloadRuntimeFor(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    DeviceValidationResult replaceRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob);
    DeviceValidationResult buildSnapshot(DeviceRegistrySnapshot& snapshot, DeviceConfigBlobMap& configBlobs) const;
    void clearRuntime(DeviceId deviceId);
    void emitRuntimeStatusChanges();
    DeviceValidationResult captureRuntimeRetainedState(DeviceId deviceId, uint32_t now);

    DeviceRegistryStore& store_;
    const DeviceTypeRegistry& typeRegistry_;
    IDeviceIdSource& idSource_;
    RetainedStateStore* retainedStateStore_{nullptr};
    DeviceRegistryEventReporter eventReporter_{};
    DeviceRegistryPersistenceCoordinator persistence_{};
    DeviceRuntimeMap runtimes_{};
    uint32_t registryRevision_{0};
};

} // namespace ewfm
