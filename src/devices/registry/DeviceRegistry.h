#pragma once

#include "devices/core/DeviceIdGenerator.h"
#include "devices/core/DeviceTypes.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "devices/registry/RetainedStateStore.h"

#include <map>
#include <memory>
#include <vector>

namespace ewfm {

struct DeviceCreateRequest {
    DeviceTypeId typeId{0};
    std::string name{};
    std::string configPayload{};
    uint32_t configVersion{0};
    bool enabled{true};
    bool hasParent{false};
    DeviceId parentDeviceId{0};
    DevicePersistencePolicy persistencePolicy{DevicePersistencePolicy::Immediate};
};

struct DeviceMutationResult {
    DeviceValidationResult validation{};
    bool pendingPersistence{false};

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

    DeviceRegistry(DeviceRegistryStore& store, const DeviceTypeRegistry& typeRegistry, IDeviceIdSource& idSource);

    DeviceValidationResult begin(uint32_t now = 0);
    void tick(uint32_t now);
    void tickFastLoop(uint32_t now);
    void tick100ms(uint32_t now);
    void tick1s(uint32_t now);

    const DeviceRegistrySnapshot& snapshot() const;
    uint32_t registryRevision() const;
    bool hasPendingPersistence() const;

    std::vector<DeviceRecord> list() const;
    const DeviceRecord* find(DeviceId deviceId) const;
    IDeviceRuntime* runtime(DeviceId deviceId);

    DeviceCreateResult create(const DeviceCreateRequest& request, uint32_t now);
    DeviceMutationResult rename(DeviceId deviceId, const std::string& name, uint32_t now,
                                DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed);
    DeviceMutationResult updateConfig(DeviceId deviceId, const std::string& configPayload, uint32_t configVersion, uint32_t now,
                                      DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed);
    DeviceMutationResult setEnabled(DeviceId deviceId, bool enabled, uint32_t now,
                                    DevicePersistencePolicy policy = DevicePersistencePolicy::Delayed);
    DeviceMutationResult remove(DeviceId deviceId, uint32_t now, DevicePersistencePolicy policy = DevicePersistencePolicy::Immediate);
    DeviceMutationResult command(const DeviceCommand& command, uint32_t now);
    DeviceValidationResult flushNow();

private:
    struct RuntimeEntry {
        const DeviceTypeDescriptor* descriptor{nullptr};
        std::unique_ptr<IDeviceRuntime> runtime{};
    };

    static constexpr uint32_t kDirtyTimestampUnset = 0xFFFFFFFFUL;

    DeviceValidationResult validateSnapshot(const DeviceRegistrySnapshot& snapshot) const;
    DeviceValidationResult validateRecord(const DeviceRecord& record, const DeviceTypeDescriptor& descriptor) const;
    DeviceValidationResult validateParent(const DeviceRegistrySnapshot& snapshot, const DeviceRecord& record) const;
    DeviceValidationResult persistIfNeeded(const DeviceRegistrySnapshot& snapshot, DevicePersistencePolicy policy);
    DeviceValidationResult reloadRuntimeFor(DeviceId deviceId);
    void clearRuntime(DeviceId deviceId);
    void clearRuntimeIfDisabled(DeviceId deviceId);
    void markDirty(uint32_t now);
    void markClean();

    DeviceRegistryStore& store_;
    const DeviceTypeRegistry& typeRegistry_;
    IDeviceIdSource& idSource_;
    DeviceRegistrySnapshot snapshot_{};
    std::map<DeviceId, RuntimeEntry> runtimes_{};
    uint32_t registryRevision_{0};
    bool dirty_{false};
    uint32_t firstDirtyAt_{kDirtyTimestampUnset};
    uint32_t dirtySince_{kDirtyTimestampUnset};
    uint32_t lastMutationAt_{0};
};

} // namespace ewfm
