#include "devices/registry/DeviceRegistry.h"

#include "debug/Debug.h"
#include "devices/registry/DeviceRegistryRelationshipOrchestrator.h"

#include <algorithm>
#include <cstdlib>

namespace ewfm {

namespace {
bool hasDuplicateNames(const DeviceRegistrySnapshot& snapshot, const std::string& name, DeviceId ignoreId = 0) {
    for (const auto& record : snapshot.records) {
        if (record.header.deviceId == ignoreId) {
            continue;
        }
        if (record.name == name) {
            return true;
        }
    }
    return false;
}

bool parseSetParentPayload(const BoundedText<kMaxDeviceEventBytes>& payload, bool& hasParent, DeviceId& parentId) {
    const std::string value(payload.view());
    constexpr const char* kPrefix = "parent=";
    if (value.rfind(kPrefix, 0) != 0) {
        return false;
    }

    const char* raw = value.c_str() + 7;
    if (*raw == '\0') {
        return false;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(raw, &end, 10);
    if (end == raw || (end != nullptr && *end != '\0')) {
        return false;
    }

    if (parsed == 0UL) {
        hasParent = false;
        parentId = 0;
        return true;
    }

    hasParent = true;
    parentId = static_cast<DeviceId>(parsed);
    return true;
}

} // namespace

DeviceRegistry::DeviceRegistry(DeviceRegistryStore& store, const DeviceTypeRegistry& typeRegistry, IDeviceIdSource& idSource,
                               RetainedStateStore* retainedStateStore, DeviceEventDispatcher* eventDispatcher)
    : store_(store), typeRegistry_(typeRegistry), idSource_(idSource), retainedStateStore_(retainedStateStore),
      eventReporter_(eventDispatcher) {}

DeviceValidationResult DeviceRegistry::begin(uint32_t now) {
    snapshot_ = {};
    runtimes_.clear();
    persistence_.reset(now);
    eventReporter_.reset();
    registryRevision_ = 0;

    const DeviceValidationResult loadResult = store_.load(snapshot_, &typeRegistry_);
    if (!loadResult.ok()) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("registry load failed: %s", loadResult.message);
        snapshot_ = {};
        return loadResult;
    }
    EWFM_DEVICE_REGISTRY_LOG_INFO("registry loaded: devices=%u", static_cast<unsigned>(snapshot_.records.size()));

    const DeviceValidationResult structureResult = validateSnapshot(snapshot_);
    if (!structureResult.ok()) {
        snapshot_ = {};
        return structureResult;
    }

    for (const auto& record : snapshot_.records) {
        const DeviceTypeDescriptor* descriptor = typeRegistry_.find(record.header.typeId);
        if (descriptor == nullptr) {
            continue;
        }
        const DeviceValidationResult configResult = validateRecord(record, *descriptor);
        if (!configResult.ok()) {
            return configResult;
        }
        const DeviceValidationResult runtimeResult = reloadRuntimeFor(record.header.deviceId);
        if (!runtimeResult.ok()) {
            return runtimeResult;
        }
        syncRuntimeParentLink(record.header.deviceId);
        if (auto* runtime = this->runtime(record.header.deviceId); runtime != nullptr) {
            if (record.enabled) {
                runtime->begin(now);
            } else {
                runtime->requestDisable();
            }
            eventReporter_.trackRuntimeStatus(record.header.deviceId, runtime->status());
        }
    }

    refreshDependentRuntimeStates(now);

    return {};
}

void DeviceRegistry::tick(uint32_t now) {
    if (!persistence_.shouldFlush(now, kPersistenceDebounceMs, kPersistenceMaxDelayMs)) {
        return;
    }

    (void)flushNow();
}

void DeviceRegistry::tickFastLoop(uint32_t now) {
    for (auto& entry : runtimes_) {
        if (entry.second.runtime != nullptr && entry.second.descriptor != nullptr && entry.second.descriptor->ticksFastLoop) {
            entry.second.runtime->tickFastLoop(now);
        }
    }
    refreshDependentRuntimeStates(now);
    emitRuntimeStatusChanges();
}

void DeviceRegistry::tick100ms(uint32_t now) {
    for (auto& entry : runtimes_) {
        if (entry.second.runtime != nullptr && entry.second.descriptor != nullptr && entry.second.descriptor->ticks100ms) {
            entry.second.runtime->tick100ms(now);
        }
    }
    refreshDependentRuntimeStates(now);
    emitRuntimeStatusChanges();
}

void DeviceRegistry::tick1s(uint32_t now) {
    for (auto& entry : runtimes_) {
        if (entry.second.runtime != nullptr && entry.second.descriptor != nullptr && entry.second.descriptor->ticks1s) {
            entry.second.runtime->tick1s(now);
        }
    }
    refreshDependentRuntimeStates(now);
    emitRuntimeStatusChanges();
}

const DeviceRegistrySnapshot& DeviceRegistry::snapshot() const {
    return snapshot_;
}

uint32_t DeviceRegistry::registryRevision() const {
    return registryRevision_;
}

bool DeviceRegistry::hasPendingPersistence() const {
    return persistence_.hasPendingPersistence();
}

bool DeviceRegistry::dirtyIndex() const {
    return persistence_.dirtyIndex();
}

std::vector<DeviceId> DeviceRegistry::dirtyConfigRecordIds() const {
    return persistence_.dirtyConfigRecordIds();
}

std::vector<DeviceId> DeviceRegistry::dirtyRetainedStateIds() const {
    return persistence_.dirtyRetainedStateIds();
}

uint32_t DeviceRegistry::firstDirtyAt() const {
    return persistence_.firstDirtyAt();
}

uint32_t DeviceRegistry::lastChangeAt() const {
    return persistence_.lastChangeAt();
}

std::vector<DeviceRecord> DeviceRegistry::list() const {
    std::vector<DeviceRecord> records = snapshot_.records;
    for (auto& record : records) {
        record.status = effectiveStatusForRecord(record);
    }
    return records;
}

const DeviceRecord* DeviceRegistry::find(DeviceId deviceId) const {
    for (const auto& record : snapshot_.records) {
        if (record.header.deviceId == deviceId) {
            return &record;
        }
    }
    return nullptr;
}

DeviceStatus DeviceRegistry::effectiveStatus(DeviceId deviceId) const {
    const DeviceRecord* record = find(deviceId);
    if (record == nullptr) {
        return DeviceStatus::Unknown;
    }
    return effectiveStatusForRecord(*record);
}

IDeviceRuntime* DeviceRegistry::runtime(DeviceId deviceId) {
    const auto it = runtimes_.find(deviceId);
    if (it == runtimes_.end()) {
        return nullptr;
    }
    return it->second.runtime.get();
}

DeviceCreateResult DeviceRegistry::create(const DeviceCreateRequest& request, uint32_t now) {
    DeviceCreateResult result{};
    const DeviceTypeDescriptor* descriptor = typeRegistry_.find(request.typeId);
    if (descriptor == nullptr) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("create rejected: unsupported type=%u", static_cast<unsigned>(request.typeId));
        result.validation = {DeviceError::UnsupportedType, "unsupported device type"};
        return result;
    }

    if (request.name.empty()) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("create rejected: empty name");
        result.validation = {DeviceError::InvalidConfig, "device name is empty"};
        return result;
    }

    if (request.name.size() > kMaxDynamicDeviceNameLength) {
        result.validation = {DeviceError::BoundsExceeded, "device name exceeds supported length"};
        return result;
    }

    if (request.configPayload.size() > kMaxDeviceConfigBytes) {
        result.validation = {DeviceError::BoundsExceeded, "device config exceeds supported size"};
        return result;
    }

    if (hasDuplicateNames(snapshot_, request.name)) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("create rejected: duplicate name=%s", request.name.c_str());
        result.validation = {DeviceError::InvalidConfig, "device name already exists"};
        return result;
    }

    DeviceId deviceId{0};
    const DeviceValidationResult idResult = assignUniqueDeviceId(
        idSource_, [this](DeviceId candidate) { return find(candidate) != nullptr; }, deviceId, kMaxDeviceIdGenerationAttempts);
    if (!idResult.ok()) {
        result.validation = idResult;
        return result;
    }

    DeviceRecord record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = deviceId;
    record.header.typeId = request.typeId;
    record.header.configVersion = request.configVersion != 0 ? request.configVersion : descriptor->currentConfigVersion;
    record.header.configRevision = 1;
    record.header.payloadLength = static_cast<uint32_t>(request.configPayload.size());
    record.header.payloadChecksum = 0;
    record.name = request.name;
    record.enabled = request.enabled;
    record.hasParent = request.hasParent;
    record.parentDeviceId = request.parentDeviceId;
    record.persistencePolicy = request.persistencePolicy;
    record.status = request.enabled ? DeviceStatus::Creating : DeviceStatus::Disabled;
    record.configPayload = request.configPayload;

    DeviceRegistrySnapshot next = snapshot_;
    next.records.push_back(record);
    next.indexEntries.push_back({deviceId, request.typeId});

    const DeviceValidationResult structureResult = validateSnapshot(next);
    if (!structureResult.ok()) {
        result.validation = structureResult;
        return result;
    }

    const DeviceValidationResult recordResult = validateRecord(record, *descriptor);
    if (!recordResult.ok()) {
        result.validation = recordResult;
        return result;
    }

    result.deviceId = deviceId;
    if (request.persistencePolicy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(next, request.persistencePolicy);
        if (!persistResult.ok()) {
            result.validation = persistResult;
            return result;
        }
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.markIndexDirty(now);
        persistence_.markConfigDirty(deviceId, now);
    }

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = reloadRuntimeFor(deviceId);
    if (!result.validation.ok()) {
        return result;
    }
    syncRuntimeParentLink(deviceId);
    auto* runtimePtr = this->runtime(deviceId);
    if (runtimePtr != nullptr) {
        if (record.enabled) {
            runtimePtr->begin(now);
        } else {
            runtimePtr->requestDisable();
        }
    }

    refreshDependentRuntimeStates(now);
    emitRuntimeStatusChanges();

    if (auto* runtimePtr = this->runtime(deviceId); runtimePtr != nullptr) {
        DeviceEvent created{};
        created.kind = DeviceEventKind::DeviceCreated;
        created.registryRevision = registryRevision_;
        created.configRevision = record.header.configRevision;
        created.deviceId = deviceId;
        created.typeId = record.header.typeId;
        created.status = runtimePtr->status();
        created.pendingPersistence = persistence_.hasPendingPersistence();
        DeviceRegistryEventReporter::setEventDetail(created, "device created");
        eventReporter_.emit(created);

        DeviceEvent accepted{};
        accepted.kind = DeviceEventKind::CommandAccepted;
        accepted.registryRevision = registryRevision_;
        accepted.configRevision = record.header.configRevision;
        accepted.deviceId = deviceId;
        accepted.typeId = record.header.typeId;
        accepted.status = runtimePtr->status();
        accepted.pendingPersistence = persistence_.hasPendingPersistence();
        accepted.commandAccepted = true;
        DeviceRegistryEventReporter::setEventDetail(accepted, "create");
        eventReporter_.emit(accepted);
        eventReporter_.trackRuntimeStatus(deviceId, runtimePtr->status());
    }

    result.validation = {};
    EWFM_DEVICE_REGISTRY_LOG_INFO("device created id=%u type=%u policy=%u pending=%d", static_cast<unsigned>(deviceId),
                                  static_cast<unsigned>(record.header.typeId), static_cast<unsigned>(request.persistencePolicy),
                                  static_cast<int>(result.pendingPersistence));
    return result;
}

DeviceCreateResult DeviceRegistry::command(const DeviceCreateRequest& request, uint32_t now) {
    const DeviceCreateResult result = create(request, now);
    if (!result.ok()) {
        DeviceEvent rejected{};
        rejected.kind = DeviceEventKind::CommandRejected;
        rejected.registryRevision = registryRevision_;
        rejected.deviceId = 0;
        rejected.commandAccepted = false;
        DeviceRegistryEventReporter::setEventDetail(rejected, result.validation.message);
        eventReporter_.emit(rejected);
    }
    return result;
}

DeviceMutationResult DeviceRegistry::rename(DeviceId deviceId, const std::string& name, uint32_t now, DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    if (name.empty()) {
        result.validation = {DeviceError::InvalidConfig, "device name is empty"};
        return result;
    }
    if (name.size() > kMaxDynamicDeviceNameLength) {
        result.validation = {DeviceError::BoundsExceeded, "device name exceeds supported length"};
        return result;
    }
    if (hasDuplicateNames(snapshot_, name, deviceId)) {
        result.validation = {DeviceError::InvalidConfig, "device name already exists"};
        return result;
    }

    auto it = std::find_if(snapshot_.records.begin(), snapshot_.records.end(),
                           [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    if (it == snapshot_.records.end()) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    DeviceRegistrySnapshot next = snapshot_;
    auto nextIt = std::find_if(next.records.begin(), next.records.end(),
                               [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    nextIt->name = name;

    const DeviceValidationResult structureResult = validateSnapshot(next);
    if (!structureResult.ok()) {
        result.validation = structureResult;
        return result;
    }

    if (policy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(next, policy);
        if (!persistResult.ok()) {
            result.validation = persistResult;
            return result;
        }
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.markIndexDirty(now);
        persistence_.markConfigDirty(deviceId, now);
    }

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.registryRevision = registryRevision_;
    updated.configRevision = nextIt->header.configRevision;
    updated.deviceId = deviceId;
    updated.typeId = nextIt->header.typeId;
    updated.status = effectiveStatus(deviceId);
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, "renamed");
    eventReporter_.emit(updated);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.configRevision = nextIt->header.configRevision;
    accepted.deviceId = deviceId;
    accepted.typeId = nextIt->header.typeId;
    accepted.status = updated.status;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, "rename");
    eventReporter_.emit(accepted);
    return result;
}

DeviceMutationResult DeviceRegistry::updateConfig(DeviceId deviceId, const std::string& configPayload, uint32_t configVersion, uint32_t now,
                                                  DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    if (configPayload.size() > kMaxDeviceConfigBytes) {
        result.validation = {DeviceError::BoundsExceeded, "device config exceeds supported size"};
        return result;
    }

    auto it = std::find_if(snapshot_.records.begin(), snapshot_.records.end(),
                           [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    if (it == snapshot_.records.end()) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    const DeviceTypeDescriptor* descriptor = typeRegistry_.find(it->header.typeId);
    if (descriptor == nullptr) {
        result.validation = {DeviceError::UnsupportedType, "unsupported device type"};
        return result;
    }

    DeviceRegistrySnapshot next = snapshot_;
    auto nextIt = std::find_if(next.records.begin(), next.records.end(),
                               [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    nextIt->header.configVersion = configVersion != 0 ? configVersion : descriptor->currentConfigVersion;
    nextIt->header.configRevision += 1;
    nextIt->header.payloadLength = static_cast<uint32_t>(configPayload.size());
    nextIt->configPayload = configPayload;

    const DeviceValidationResult recordResult = validateRecord(*nextIt, *descriptor);
    if (!recordResult.ok()) {
        result.validation = recordResult;
        return result;
    }

    if (policy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(next, policy);
        if (!persistResult.ok()) {
            result.validation = persistResult;
            return result;
        }
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.markIndexDirty(now);
        persistence_.markConfigDirty(deviceId, now);
    }

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    if (const auto runtimeIt = runtimes_.find(deviceId); runtimeIt != runtimes_.end() && runtimeIt->second.runtime != nullptr) {
        runtimeIt->second.runtime->requestReconfigure();
    }
    refreshDependentRuntimeStates(now);
    emitRuntimeStatusChanges();

    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.registryRevision = registryRevision_;
    updated.configRevision = nextIt->header.configRevision;
    updated.deviceId = deviceId;
    updated.typeId = nextIt->header.typeId;
    updated.status = effectiveStatus(deviceId);
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, "config updated");
    eventReporter_.emit(updated);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.configRevision = nextIt->header.configRevision;
    accepted.deviceId = deviceId;
    accepted.typeId = nextIt->header.typeId;
    accepted.status = updated.status;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, "update_config");
    eventReporter_.emit(accepted);
    return result;
}

DeviceMutationResult DeviceRegistry::setParent(DeviceId deviceId, bool hasParent, DeviceId parentDeviceId, uint32_t now,
                                               DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    auto it = std::find_if(snapshot_.records.begin(), snapshot_.records.end(),
                           [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    if (it == snapshot_.records.end()) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    if (policy != DevicePersistencePolicy::Immediate) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("setParent rejected: non-immediate policy=%u", static_cast<unsigned>(policy));
        result.validation = {DeviceError::InvalidConfig, "parent reassignment requires immediate persistence"};
        return result;
    }

    DeviceRegistrySnapshot next = snapshot_;
    auto nextIt = std::find_if(next.records.begin(), next.records.end(),
                               [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    nextIt->hasParent = hasParent;
    nextIt->parentDeviceId = hasParent ? parentDeviceId : 0;

    const DeviceValidationResult structureResult = validateSnapshot(next);
    if (!structureResult.ok()) {
        result.validation = structureResult;
        return result;
    }

    const DeviceValidationResult persistResult = persistIfNeeded(next, policy);
    if (!persistResult.ok()) {
        result.validation = persistResult;
        return result;
    }

    snapshot_ = std::move(next);
    ++registryRevision_;
    persistence_.clearConfigDirtyAfterImmediateFlush();

    syncRuntimeParentLink(deviceId);
    if (auto* runtimePtr = runtime(deviceId); runtimePtr != nullptr) {
        runtimePtr->requestReconfigure();
    }

    refreshDependentRuntimeStates(now);
    emitRuntimeStatusChanges();

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.registryRevision = registryRevision_;
    updated.configRevision = nextIt->header.configRevision;
    updated.deviceId = deviceId;
    updated.typeId = nextIt->header.typeId;
    updated.status = effectiveStatus(deviceId);
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, "parent reassigned");
    eventReporter_.emit(updated);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.configRevision = nextIt->header.configRevision;
    accepted.deviceId = deviceId;
    accepted.typeId = nextIt->header.typeId;
    accepted.status = updated.status;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, "set_parent");
    eventReporter_.emit(accepted);
    EWFM_DEVICE_REGISTRY_LOG_INFO("parent reassigned device=%u hasParent=%d parent=%u", static_cast<unsigned>(deviceId),
                                  static_cast<int>(hasParent), static_cast<unsigned>(hasParent ? parentDeviceId : 0));
    return result;
}

DeviceMutationResult DeviceRegistry::setEnabled(DeviceId deviceId, bool enabled, uint32_t now, DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    auto it = std::find_if(snapshot_.records.begin(), snapshot_.records.end(),
                           [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    if (it == snapshot_.records.end()) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    DeviceRegistrySnapshot next = snapshot_;
    auto nextIt = std::find_if(next.records.begin(), next.records.end(),
                               [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    nextIt->enabled = enabled;
    nextIt->status = enabled ? DeviceStatus::Creating : DeviceStatus::Disabled;

    const DeviceValidationResult structureResult = validateSnapshot(next);
    if (!structureResult.ok()) {
        result.validation = structureResult;
        return result;
    }

    if (policy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(next, policy);
        if (!persistResult.ok()) {
            result.validation = persistResult;
            return result;
        }
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.markIndexDirty(now);
        persistence_.markConfigDirty(deviceId, now);
    }

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    if (enabled) {
        if (runtime(deviceId) == nullptr) {
            result.validation = reloadRuntimeFor(deviceId);
            if (!result.validation.ok()) {
                return result;
            }
        }
        syncRuntimeParentLink(deviceId);
        const auto runtimePtr = runtime(deviceId);
        if (runtimePtr != nullptr) {
            runtimePtr->requestReconfigure();
        }
    } else {
        if (const auto runtimePtr = runtime(deviceId); runtimePtr != nullptr) {
            runtimePtr->requestDisable();
        }
    }
    refreshDependentRuntimeStates(now);
    emitRuntimeStatusChanges();

    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.registryRevision = registryRevision_;
    updated.configRevision = nextIt->header.configRevision;
    updated.deviceId = deviceId;
    updated.typeId = nextIt->header.typeId;
    updated.status = effectiveStatus(deviceId);
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, enabled ? "enabled" : "disabled");
    eventReporter_.emit(updated);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.configRevision = nextIt->header.configRevision;
    accepted.deviceId = deviceId;
    accepted.typeId = nextIt->header.typeId;
    accepted.status = updated.status;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, enabled ? "enable" : "disable");
    eventReporter_.emit(accepted);
    return result;
}

DeviceMutationResult DeviceRegistry::remove(DeviceId deviceId, uint32_t now, DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    auto it = std::find_if(snapshot_.records.begin(), snapshot_.records.end(),
                           [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; });
    if (it == snapshot_.records.end()) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    result.dependentChildDeviceIds = childDeviceIds(deviceId);
    if (!result.dependentChildDeviceIds.empty()) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("delete rejected: device=%u has %u children", static_cast<unsigned>(deviceId),
                                      static_cast<unsigned>(result.dependentChildDeviceIds.size()));
        result.validation = {DeviceError::InvalidRelationship, "device has dependent child devices"};
        return result;
    }

    DeviceRegistrySnapshot next = snapshot_;
    next.records.erase(std::remove_if(next.records.begin(), next.records.end(),
                                      [deviceId](const DeviceRecord& record) { return record.header.deviceId == deviceId; }),
                       next.records.end());
    next.indexEntries.erase(std::remove_if(next.indexEntries.begin(), next.indexEntries.end(),
                                           [deviceId](const DeviceIndexEntry& entry) { return entry.deviceId == deviceId; }),
                            next.indexEntries.end());

    const DeviceValidationResult structureResult = validateSnapshot(next);
    if (!structureResult.ok() && structureResult.error != DeviceError::MissingRecord) {
        result.validation = structureResult;
        return result;
    }

    if (policy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(next, policy);
        if (!persistResult.ok()) {
            result.validation = persistResult;
            return result;
        }
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        persistence_.markIndexDirty(now);
        persistence_.markConfigDirty(deviceId, now);
    }

    clearRuntime(deviceId);
    refreshDependentRuntimeStates(now);
    emitRuntimeStatusChanges();

    DeviceEvent deleted{};
    deleted.kind = DeviceEventKind::DeviceDeleted;
    deleted.registryRevision = registryRevision_;
    deleted.deviceId = deviceId;
    deleted.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(deleted, "deleted");
    eventReporter_.emit(deleted);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.deviceId = deviceId;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, "delete");
    eventReporter_.emit(accepted);
    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    EWFM_DEVICE_REGISTRY_LOG_INFO("device deleted id=%u pending=%d", static_cast<unsigned>(deviceId),
                                  static_cast<int>(result.pendingPersistence));
    return result;
}

DeviceMutationResult DeviceRegistry::setRetainedState(DeviceId deviceId, const std::string& payload, uint32_t now,
                                                      DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    if (payload.size() > kMaxRetainedStateBytes) {
        result.validation = {DeviceError::BoundsExceeded, "retained state exceeds supported size"};
        return result;
    }

    const DeviceRecord* record = find(deviceId);
    if (record == nullptr) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    const DeviceTypeDescriptor* descriptor = typeRegistry_.find(record->header.typeId);
    if (descriptor == nullptr) {
        result.validation = {DeviceError::UnsupportedType, "unsupported device type"};
        return result;
    }

    if (!descriptor->supportsRetainedState) {
        result.validation = {DeviceError::InvalidConfig, "device type does not support retained state"};
        return result;
    }

    RetainedStateRecord retained{};
    retained.deviceId = deviceId;
    retained.payload = payload;

    if (policy == DevicePersistencePolicy::Immediate) {
        if (retainedStateStore_ == nullptr) {
            result.validation = {DeviceError::StorageError, "retained state store is unavailable"};
            return result;
        }

        const DeviceValidationResult persistResult = retainedStateStore_->save(retained);
        if (!persistResult.ok()) {
            result.validation = persistResult;
            return result;
        }
        persistence_.clearRetainedTracking(deviceId);
        if (!persistence_.hasAnyPersistenceWork()) {
            persistence_.markClean();
        }
        result.pendingPersistence = persistence_.hasPendingPersistence();
        result.validation = {};
        DeviceEvent updated{};
        updated.kind = DeviceEventKind::RetainedStateChanged;
        updated.registryRevision = registryRevision_;
        updated.deviceId = deviceId;
        updated.typeId = record->header.typeId;
        updated.pendingPersistence = persistence_.hasPendingPersistence();
        DeviceRegistryEventReporter::setEventDetail(updated, "retained state persisted");
        eventReporter_.emit(updated);
        return result;
    }

    persistence_.pendingRetainedStateRecords()[deviceId] = retained;
    persistence_.markRetainedDirty(deviceId, now);
    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    DeviceEvent updated{};
    updated.kind = DeviceEventKind::RetainedStateChanged;
    updated.registryRevision = registryRevision_;
    updated.deviceId = deviceId;
    updated.typeId = record->header.typeId;
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, "retained state updated");
    eventReporter_.emit(updated);
    return result;
}

DeviceMutationResult DeviceRegistry::command(const DeviceCommand& command, uint32_t now) {
    if (!command.valid()) {
        DeviceMutationResult result{{DeviceError::BoundsExceeded, "command payload exceeds supported size"}, false};
        DeviceEvent rejected{};
        rejected.kind = DeviceEventKind::CommandRejected;
        rejected.registryRevision = registryRevision_;
        rejected.deviceId = command.deviceId;
        rejected.commandAccepted = false;
        DeviceRegistryEventReporter::setEventDetail(rejected, result.validation.message);
        eventReporter_.emit(rejected);
        return result;
    }

    switch (command.type) {
    case DeviceCommandType::Rename:
        return rename(command.deviceId, std::string(command.payload.view()), now, command.persistencePolicy);
    case DeviceCommandType::Enable:
        return setEnabled(command.deviceId, true, now, command.persistencePolicy);
    case DeviceCommandType::Disable:
        return setEnabled(command.deviceId, false, now, command.persistencePolicy);
    case DeviceCommandType::Delete:
        return remove(command.deviceId, now, command.persistencePolicy);
    case DeviceCommandType::UpdateConfig:
        return updateConfig(command.deviceId, std::string(command.payload.view()), 0, now, command.persistencePolicy);
    case DeviceCommandType::SetStatus: {
        DeviceMutationResult result{};
        auto runtime = this->runtime(command.deviceId);
        if (runtime == nullptr) {
            result.validation = {DeviceError::MissingRecord, "device runtime not available"};
            DeviceEvent rejected{};
            rejected.kind = DeviceEventKind::CommandRejected;
            rejected.registryRevision = registryRevision_;
            rejected.deviceId = command.deviceId;
            rejected.commandAccepted = false;
            DeviceRegistryEventReporter::setEventDetail(rejected, result.validation.message);
            eventReporter_.emit(rejected);
            return result;
        }
        if (!runtime->handleCommand(command)) {
            result.validation = {DeviceError::InvalidCommand, "command rejected by runtime"};
            DeviceEvent rejected{};
            rejected.kind = DeviceEventKind::CommandRejected;
            rejected.registryRevision = registryRevision_;
            rejected.deviceId = command.deviceId;
            rejected.typeId = find(command.deviceId) != nullptr ? find(command.deviceId)->header.typeId : 0;
            rejected.commandAccepted = false;
            DeviceRegistryEventReporter::setEventDetail(rejected, result.validation.message);
            eventReporter_.emit(rejected);
            return result;
        }
        refreshDependentRuntimeStates(now);
        emitRuntimeStatusChanges();
        DeviceEvent accepted{};
        accepted.kind = DeviceEventKind::CommandAccepted;
        accepted.registryRevision = registryRevision_;
        accepted.deviceId = command.deviceId;
        accepted.typeId = find(command.deviceId) != nullptr ? find(command.deviceId)->header.typeId : 0;
        accepted.status = runtime->status();
        accepted.commandAccepted = true;
        accepted.pendingPersistence = persistence_.hasPendingPersistence();
        DeviceRegistryEventReporter::setEventDetail(accepted, "runtime command");
        eventReporter_.emit(accepted);
        result.validation = {};
        return result;
    }
    case DeviceCommandType::SetParent: {
        bool hasParent = false;
        DeviceId parentId = 0;
        if (!parseSetParentPayload(command.payload, hasParent, parentId)) {
            DeviceMutationResult result{{DeviceError::InvalidCommand, "set_parent payload must be parent=<device_id|0>"}, false};
            DeviceEvent rejected{};
            rejected.kind = DeviceEventKind::CommandRejected;
            rejected.registryRevision = registryRevision_;
            rejected.deviceId = command.deviceId;
            rejected.commandAccepted = false;
            DeviceRegistryEventReporter::setEventDetail(rejected, result.validation.message);
            eventReporter_.emit(rejected);
            return result;
        }
        return setParent(command.deviceId, hasParent, parentId, now, command.persistencePolicy);
    }
    case DeviceCommandType::Create:
    case DeviceCommandType::Custom:
    case DeviceCommandType::None:
        break;
    }

    DeviceMutationResult result{{DeviceError::InvalidCommand, "unsupported command"}, false};
    DeviceEvent rejected{};
    rejected.kind = DeviceEventKind::CommandRejected;
    rejected.registryRevision = registryRevision_;
    rejected.deviceId = command.deviceId;
    rejected.commandAccepted = false;
    DeviceRegistryEventReporter::setEventDetail(rejected, result.validation.message);
    eventReporter_.emit(rejected);
    return result;
}

DeviceValidationResult DeviceRegistry::flushNow() {
    if (!persistence_.hasPendingPersistence()) {
        return {};
    }
    EWFM_DEVICE_REGISTRY_LOG_DEBUG("flush start: dirtyIndex=%d configDirty=%u retainedDirty=%u",
                                   static_cast<int>(persistence_.dirtyIndex()),
                                   static_cast<unsigned>(persistence_.dirtyConfigRecordIdsRef().size()),
                                   static_cast<unsigned>(persistence_.dirtyRetainedStateIdsRef().size()));

    if (persistence_.hasConfigPersistenceWork()) {
        const DeviceValidationResult saveResult = store_.save(snapshot_);
        if (!saveResult.ok()) {
            return saveResult;
        }
        for (const DeviceId deviceId : persistence_.dirtyConfigRecordIdsRef()) {
            const DeviceRecord* record = find(deviceId);
            if (record == nullptr) {
                continue;
            }
            DeviceEvent persisted{};
            persisted.kind = DeviceEventKind::ConfigPersisted;
            persisted.registryRevision = registryRevision_;
            persisted.configRevision = record->header.configRevision;
            persisted.deviceId = deviceId;
            persisted.typeId = record->header.typeId;
            persisted.status = effectiveStatusForRecord(*record);
            persisted.pendingPersistence = false;
            DeviceRegistryEventReporter::setEventDetail(persisted, "config persisted");
            eventReporter_.emit(persisted);
        }
        persistence_.clearConfigDirtyAfterPersisted();
    }

    if (persistence_.hasRetainedPersistenceWork()) {
        if (retainedStateStore_ == nullptr) {
            return {DeviceError::StorageError, "retained state store is unavailable"};
        }

        for (const DeviceId deviceId : persistence_.dirtyRetainedStateIdsRef()) {
            auto& pending = persistence_.pendingRetainedStateRecords();
            const auto pendingIt = pending.find(deviceId);
            if (pendingIt == pending.end()) {
                continue;
            }

            const DeviceValidationResult saveResult = retainedStateStore_->save(pendingIt->second);
            if (!saveResult.ok()) {
                return saveResult;
            }

            const DeviceRecord* record = find(deviceId);
            if (record != nullptr) {
                DeviceEvent persisted{};
                persisted.kind = DeviceEventKind::RetainedStateChanged;
                persisted.registryRevision = registryRevision_;
                persisted.deviceId = deviceId;
                persisted.typeId = record->header.typeId;
                persisted.status = effectiveStatusForRecord(*record);
                persisted.pendingPersistence = false;
                DeviceRegistryEventReporter::setEventDetail(persisted, "retained state persisted");
                eventReporter_.emit(persisted);
            }

            persistence_.markRetainedPersisted(deviceId);
        }
        persistence_.clearRetainedDirtyAfterPersisted();
    }

    if (persistence_.hasAnyPersistenceWork()) {
        return {};
    }

    DeviceEvent cleared{};
    cleared.kind = DeviceEventKind::PersistencePendingCleared;
    cleared.registryRevision = registryRevision_;
    cleared.pendingPersistence = false;
    DeviceRegistryEventReporter::setEventDetail(cleared, "pending persistence cleared");
    eventReporter_.emit(cleared);
    persistence_.markClean();
    EWFM_DEVICE_REGISTRY_LOG_INFO("flush complete");
    return {};
}

DeviceValidationResult DeviceRegistry::validateSnapshot(const DeviceRegistrySnapshot& snapshot) const {
    if (snapshot.records.size() > kMaxDynamicDevices || snapshot.indexEntries.size() > kMaxDynamicDevices) {
        return {DeviceError::BoundsExceeded, "registry exceeds supported device count"};
    }

    if (!snapshot.indexEntries.empty() && snapshot.indexEntries.size() != snapshot.records.size()) {
        return {DeviceError::InvalidConfig, "index entry and record counts differ"};
    }

    std::map<DeviceId, const DeviceRecord*> recordsById;
    for (const auto& record : snapshot.records) {
        if (record.header.deviceId == 0) {
            return {DeviceError::InvalidDeviceId, "device id is invalid"};
        }
        if (record.name.empty() || record.name.size() > kMaxDynamicDeviceNameLength) {
            return {DeviceError::BoundsExceeded, "device name is invalid"};
        }
        if (record.configPayload.size() > kMaxDeviceConfigBytes) {
            return {DeviceError::BoundsExceeded, "device config exceeds supported size"};
        }
        if (recordsById.find(record.header.deviceId) != recordsById.end()) {
            return {DeviceError::DuplicateDeviceId, "duplicate device id"};
        }
        recordsById.emplace(record.header.deviceId, &record);
    }

    for (const auto& entry : snapshot.indexEntries) {
        const auto it = recordsById.find(entry.deviceId);
        if (it == recordsById.end()) {
            return {DeviceError::MissingRecord, "missing device record for index entry"};
        }
        if (it->second->header.typeId != entry.typeId) {
            return {DeviceError::InvalidConfig, "index entry type does not match device record"};
        }
    }

    for (const auto& record : snapshot.records) {
        const DeviceTypeDescriptor* descriptor = typeRegistry_.find(record.header.typeId);
        if (descriptor == nullptr) {
            return {DeviceError::UnsupportedType, "unsupported device type"};
        }
        const DeviceValidationResult recordResult = validateRecord(record, *descriptor);
        if (!recordResult.ok()) {
            return recordResult;
        }
        const DeviceValidationResult parentResult = validateParent(snapshot, record);
        if (!parentResult.ok()) {
            return parentResult;
        }
    }

    const DeviceValidationResult graphResult = validateAcyclicParentGraph(snapshot);
    if (!graphResult.ok()) {
        return graphResult;
    }

    return {};
}

DeviceValidationResult DeviceRegistry::validateRecord(const DeviceRecord& record, const DeviceTypeDescriptor& descriptor) const {
    if (record.header.configVersion == 0 || record.header.configVersion > descriptor.currentConfigVersion) {
        return {DeviceError::InvalidVersion, "unsupported config version"};
    }
    if (descriptor.validateConfig != nullptr) {
        return descriptor.validateConfig(record);
    }
    return {};
}

DeviceValidationResult DeviceRegistry::validateParent(const DeviceRegistrySnapshot& snapshot, const DeviceRecord& record) const {
    if (!record.hasParent) {
        return {};
    }

    if (record.parentDeviceId == record.header.deviceId) {
        return {DeviceError::InvalidRelationship, "self parent relationship is not allowed"};
    }

    const auto parentIt =
        std::find_if(snapshot.records.begin(), snapshot.records.end(),
                     [parentId = record.parentDeviceId](const DeviceRecord& candidate) { return candidate.header.deviceId == parentId; });
    if (parentIt == snapshot.records.end()) {
        return {DeviceError::InvalidRelationship, "parent device does not exist"};
    }

    const DeviceTypeDescriptor* childDescriptor = typeRegistry_.find(record.header.typeId);
    const DeviceTypeDescriptor* parentDescriptor = typeRegistry_.find(parentIt->header.typeId);
    if (childDescriptor == nullptr || parentDescriptor == nullptr) {
        return {DeviceError::UnsupportedType, "unsupported device type"};
    }
    if (!childDescriptor->compatibleParentTypes.empty() &&
        std::find(childDescriptor->compatibleParentTypes.begin(), childDescriptor->compatibleParentTypes.end(), parentIt->header.typeId) ==
            childDescriptor->compatibleParentTypes.end()) {
        return {DeviceError::InvalidRelationship, "incompatible parent type"};
    }
    if (!parentDescriptor->canHaveChildren) {
        return {DeviceError::InvalidRelationship, "parent type cannot have children"};
    }

    size_t childCount = 0;
    for (const auto& candidate : snapshot.records) {
        if (candidate.hasParent && candidate.parentDeviceId == parentIt->header.deviceId) {
            ++childCount;
        }
    }
    if (parentDescriptor->maxChildren > 0 && childCount > parentDescriptor->maxChildren) {
        return {DeviceError::InvalidRelationship, "parent child limit exceeded"};
    }
    return {};
}

DeviceValidationResult DeviceRegistry::validateAcyclicParentGraph(const DeviceRegistrySnapshot& snapshot) const {
    std::map<DeviceId, const DeviceRecord*> recordsById;
    for (const auto& record : snapshot.records) {
        recordsById.emplace(record.header.deviceId, &record);
    }

    for (const auto& record : snapshot.records) {
        if (!record.hasParent) {
            continue;
        }

        std::map<DeviceId, bool> seen;
        DeviceId parentId = record.parentDeviceId;
        while (parentId != 0) {
            if (parentId == record.header.deviceId) {
                return {DeviceError::InvalidRelationship, "cyclic parent relationship detected"};
            }
            if (seen.find(parentId) != seen.end()) {
                return {DeviceError::InvalidRelationship, "cyclic parent relationship detected"};
            }
            seen[parentId] = true;

            const auto current = recordsById.find(parentId);
            if (current == recordsById.end()) {
                break;
            }
            const DeviceRecord& parentRecord = *current->second;
            if (!parentRecord.hasParent) {
                break;
            }
            parentId = parentRecord.parentDeviceId;
        }
    }

    return {};
}

std::vector<DeviceId> DeviceRegistry::childDeviceIds(DeviceId parentId) const {
    std::vector<DeviceId> children;
    for (const auto& record : snapshot_.records) {
        if (record.hasParent && record.parentDeviceId == parentId) {
            children.push_back(record.header.deviceId);
        }
    }
    return children;
}

void DeviceRegistry::syncRuntimeParentLink(DeviceId deviceId) {
    DeviceRegistryRelationshipOrchestrator::syncRuntimeParentLink(deviceId, snapshot_, runtimes_);
}

DeviceStatus DeviceRegistry::effectiveStatusForRecord(const DeviceRecord& record) const {
    return DeviceRegistryRelationshipOrchestrator::effectiveStatusForRecord(record, snapshot_, runtimes_);
}

void DeviceRegistry::refreshDependentRuntimeStates(uint32_t now) {
    (void)now;
    DeviceRegistryRelationshipOrchestrator::refreshDependentRuntimeStates(snapshot_, runtimes_);
}

void DeviceRegistry::emitRuntimeStatusChanges() {
    for (const auto& entry : runtimes_) {
        if (entry.second.runtime == nullptr) {
            continue;
        }
        const DeviceStatus current = entry.second.runtime->status();
        const DeviceRecord* record = find(entry.first);
        const DeviceTypeId typeId = record != nullptr ? record->header.typeId : 0;
        eventReporter_.emitRuntimeStatusChangeIfNeeded(entry.first, typeId, current, registryRevision_,
                                                       persistence_.hasPendingPersistence(), "runtime status changed");
    }
}

DeviceValidationResult DeviceRegistry::persistIfNeeded(const DeviceRegistrySnapshot& snapshot, DevicePersistencePolicy policy) {
    if (policy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult saveResult = store_.save(snapshot);
        if (!saveResult.ok()) {
            return saveResult;
        }
        return {};
    }
    return {};
}

DeviceValidationResult DeviceRegistry::reloadRuntimeFor(DeviceId deviceId) {
    const DeviceRecord* record = find(deviceId);
    if (record == nullptr) {
        return {DeviceError::MissingRecord, "device not found"};
    }

    const DeviceTypeDescriptor* descriptor = typeRegistry_.find(record->header.typeId);
    if (descriptor == nullptr) {
        return {DeviceError::UnsupportedType, "unsupported device type"};
    }
    if (descriptor->createRuntime == nullptr) {
        return {};
    }
    if (descriptor->validateConfig != nullptr) {
        const DeviceValidationResult validation = descriptor->validateConfig(*record);
        if (!validation.ok()) {
            return validation;
        }
    }

    DeviceRuntimeSlot entry;
    entry.descriptor = descriptor;
    entry.runtime = descriptor->createRuntime(*record);
    if (entry.runtime == nullptr) {
        return {DeviceError::StorageError, "failed to create runtime"};
    }
    runtimes_[deviceId] = std::move(entry);
    return {};
}

void DeviceRegistry::clearRuntime(DeviceId deviceId) {
    const auto it = runtimes_.find(deviceId);
    if (it != runtimes_.end() && it->second.runtime != nullptr) {
        if (IDeviceRuntime* parent = it->second.runtime->parentRuntime(); parent != nullptr) {
            parent->detachChildRuntime(it->second.runtime.get());
        }
    }
    runtimes_.erase(deviceId);
    eventReporter_.clearRuntimeStatus(deviceId);
}

void DeviceRegistry::clearRuntimeIfDisabled(DeviceId deviceId) {
    const DeviceRecord* record = find(deviceId);
    if (record != nullptr && !record->enabled) {
        clearRuntime(deviceId);
    }
}

} // namespace ewfm
