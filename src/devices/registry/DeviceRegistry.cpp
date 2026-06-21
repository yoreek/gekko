#include "devices/registry/DeviceRegistry.h"

#include "debug/Debug.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/registry/DeviceRegistryBinaryCodec.h"

#include <algorithm>
#include <cstring>

namespace ewfm {

namespace {
DeviceRegistryEntry recordFromRuntime(const IDeviceRuntime& runtime) {
    DeviceConfigBlob configBlob{};
    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = runtime.deviceId();
    record.header.typeId = runtime.typeId();
    record.header.configVersion = runtime.configVersion();
    record.header.configRevision = runtime.configRevision();
    record.header.payloadChecksum = 0;
    if (runtime.serializeConfigBlob(configBlob)) {
        record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    }
    record.depCount = runtime.dependencyCount();
    const DeviceDependencyLink* links = runtime.dependencyLinks();
    for (uint8_t index = 0; index < record.depCount && links != nullptr && index < kMaxDeviceDependencies; ++index) {
        record.deps[index] = links[index];
    }
    record.persistencePolicy = runtime.persistencePolicy();
    record.status = runtime.status();
    return record;
}

bool hasDuplicateNames(const DeviceRuntimeMap& runtimes, const std::string& name, DeviceId ignoreId = 0) {
    for (const auto& entry : runtimes) {
        const IDeviceRuntime* runtime = entry.second.runtime.get();
        if (runtime == nullptr || runtime->deviceId() == ignoreId) {
            continue;
        }
        const char* runtimeName = runtime->name();
        if (runtimeName != nullptr && name == runtimeName) {
            return true;
        }
    }
    return false;
}

bool dependencyLinkLess(const DeviceDependencyLink& left, const DeviceDependencyLink& right) {
    if (left.role != right.role) {
        return static_cast<uint8_t>(left.role) < static_cast<uint8_t>(right.role);
    }
    return left.deviceId < right.deviceId;
}

bool sameDependencyLinks(const DeviceDependencyLink* leftLinks, uint8_t leftCount, const DeviceDependencyLink* rightLinks,
                         uint8_t rightCount) {
    if (leftCount != rightCount) {
        return false;
    }
    if (leftCount == 0U) {
        return true;
    }
    if (leftLinks == nullptr || rightLinks == nullptr) {
        return false;
    }

    std::array<DeviceDependencyLink, kMaxDeviceDependencies> left{};
    std::array<DeviceDependencyLink, kMaxDeviceDependencies> right{};
    for (uint8_t index = 0; index < leftCount && index < kMaxDeviceDependencies; ++index) {
        left[index] = leftLinks[index];
        right[index] = rightLinks[index];
    }
    std::sort(left.begin(), left.begin() + leftCount, dependencyLinkLess);
    std::sort(right.begin(), right.begin() + rightCount, dependencyLinkLess);
    for (uint8_t index = 0; index < leftCount; ++index) {
        if (left[index].role != right[index].role || left[index].deviceId != right[index].deviceId) {
            return false;
        }
    }
    return true;
}

DeviceMutationResult handleUpdateConfigCommand(DeviceRegistry& registry, const DeviceCommand& command, uint32_t now) {
    BoundedBlob<kMaxDeviceConfigBytes> payload{};
    (void)payload.assign(reinterpret_cast<const uint8_t*>(command.payload.view().data()), command.payload.view().size());
    return registry.updateConfig(command.deviceId, payload, 0, now, command.persistencePolicy);
}

} // namespace

DeviceRegistry::DeviceRegistry(DeviceRegistryStore& store, const DeviceTypeRegistry& typeRegistry, IDeviceIdSource& idSource,
                               RetainedStateStore* retainedStateStore, DeviceEventDispatcher* eventDispatcher)
    : store_(store), typeRegistry_(typeRegistry), idSource_(idSource), retainedStateStore_(retainedStateStore),
      eventReporter_(eventDispatcher) {}

DeviceValidationResult DeviceRegistry::begin(uint32_t now) {
    runtimes_.clear();
    persistence_.reset(now);
    eventReporter_.reset();
    registryRevision_ = 0;

    DeviceRegistrySnapshot loadedSnapshot{};
    DeviceConfigBlobMap loadedConfigBlobs{};
    const DeviceValidationResult loadResult = store_.load(loadedSnapshot, loadedConfigBlobs, &typeRegistry_);
    if (!loadResult.ok()) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("registry load failed: %s", loadResult.message);
        return loadResult;
    }
    EWFM_DEVICE_REGISTRY_LOG_INFO("registry loaded: devices=%u", static_cast<unsigned>(loadedSnapshot.records.size()));

    const DeviceValidationResult structureResult = validateSnapshot(loadedSnapshot, loadedConfigBlobs);
    if (!structureResult.ok()) {
        return structureResult;
    }

    for (const auto& record : loadedSnapshot.records) {
        const DeviceTypeDescriptor* descriptor = typeRegistry_.find(record.header.typeId);
        if (descriptor == nullptr) {
            continue;
        }
        const auto configIt = loadedConfigBlobs.find(record.header.deviceId);
        if (configIt == loadedConfigBlobs.end()) {
            return {DeviceError::MissingRecord, "missing device config"};
        }
        const DeviceValidationResult configResult = validateRecord(record, *descriptor, configIt->second);
        if (!configResult.ok()) {
            return configResult;
        }
        const DeviceValidationResult runtimeResult = reloadRuntimeFor(record, configIt->second);
        if (!runtimeResult.ok()) {
            return runtimeResult;
        }
        syncRuntimeDependencyLinks(record.header.deviceId);
        if (auto* runtime = this->runtime(record.header.deviceId); runtime != nullptr) {
            if (runtime->enabled()) {
                runtime->begin(now);
            } else {
                runtime->requestDisable();
            }
            eventReporter_.trackRuntimeStatus(record.header.deviceId, runtime->status());
        }
    }

    // Run a second pass after all runtimes exist so late-loaded dependencies are wired back into earlier dependents.
    for (const auto& record : loadedSnapshot.records) {
        syncRuntimeDependencyLinks(record.header.deviceId);
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
    captureDirtyRuntimeRetainedStates(now);
    emitRuntimeStatusChanges();
}

void DeviceRegistry::tick100ms(uint32_t now) {
    for (auto& entry : runtimes_) {
        if (entry.second.runtime != nullptr && entry.second.descriptor != nullptr && entry.second.descriptor->ticks100ms) {
            entry.second.runtime->tick100ms(now);
        }
    }
    refreshDependentRuntimeStates(now);
    captureDirtyRuntimeRetainedStates(now);
    emitRuntimeStatusChanges();
}

void DeviceRegistry::tick1s(uint32_t now) {
    for (auto& entry : runtimes_) {
        if (entry.second.runtime != nullptr && entry.second.descriptor != nullptr && entry.second.descriptor->ticks1s) {
            entry.second.runtime->tick1s(now);
        }
    }
    refreshDependentRuntimeStates(now);
    captureDirtyRuntimeRetainedStates(now);
    emitRuntimeStatusChanges();
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

std::vector<DeviceRegistryEntry> DeviceRegistry::list() const {
    std::vector<DeviceRegistryEntry> records;
    records.reserve(runtimes_.size());
    for (const auto& entry : runtimes_) {
        if (entry.second.runtime == nullptr) {
            continue;
        }
        DeviceRegistryEntry record = recordFromRuntime(*entry.second.runtime);
        record.status = effectiveStatusForRuntime(*entry.second.runtime);
        records.push_back(record);
    }
    return records;
}

DeviceStatus DeviceRegistry::effectiveStatus(DeviceId deviceId) const {
    const IDeviceRuntime* runtimePtr = runtime(deviceId);
    if (runtimePtr == nullptr) {
        return DeviceStatus::Unknown;
    }
    return effectiveStatusForRuntime(*runtimePtr);
}

IDeviceRuntime* DeviceRegistry::runtime(DeviceId deviceId) {
    const auto it = runtimes_.find(deviceId);
    if (it == runtimes_.end()) {
        return nullptr;
    }
    return it->second.runtime.get();
}

const IDeviceRuntime* DeviceRegistry::runtime(DeviceId deviceId) const {
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

    if (request.configBlob.size() > kMaxDeviceConfigBytes) {
        result.validation = {DeviceError::BoundsExceeded, "device config exceeds supported size"};
        return result;
    }

    if (hasDuplicateNames(runtimes_, request.name)) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("create rejected: duplicate name=%s", request.name.c_str());
        result.validation = {DeviceError::InvalidConfig, "device name already exists"};
        return result;
    }

    DeviceId deviceId{0};
    const DeviceValidationResult idResult = assignUniqueDeviceId(
        idSource_, [this](DeviceId candidate) { return runtime(candidate) != nullptr; }, deviceId, kMaxDeviceIdGenerationAttempts);
    if (!idResult.ok()) {
        result.validation = idResult;
        return result;
    }

    DeviceRegistryEntry record{};
    record.header.recordVersion = kDeviceRecordHeaderVersion;
    record.header.deviceId = deviceId;
    record.header.typeId = request.typeId;
    record.header.configVersion = request.configVersion != 0 ? request.configVersion : descriptor->currentConfigVersion;
    record.header.configRevision = 1;
    record.header.payloadLength = static_cast<uint32_t>(request.configBlob.size());
    record.header.payloadChecksum = 0;
    record.depCount = request.dependencyCount();
    const DeviceDependencyLink* requestDeps = request.dependencyLinks();
    for (uint8_t index = 0; index < request.dependencyCount() && index < kMaxDeviceDependencies && requestDeps != nullptr; ++index) {
        record.deps[index] = requestDeps[index];
    }
    record.persistencePolicy = request.persistencePolicy;
    record.status = request.enabled ? DeviceStatus::Creating : DeviceStatus::Disabled;

    DeviceRegistrySnapshot next{};
    DeviceConfigBlobMap nextConfigBlobs{};
    const DeviceValidationResult snapshotResult = buildSnapshot(next, nextConfigBlobs);
    if (!snapshotResult.ok()) {
        result.validation = snapshotResult;
        return result;
    }
    next.records.push_back(record);
    next.indexEntries.push_back({deviceId, request.typeId});
    nextConfigBlobs[deviceId] = request.configBlob;

    const DeviceValidationResult recordResult = validateRecord(record, *descriptor, request.configBlob);
    if (!recordResult.ok()) {
        result.validation = recordResult;
        return result;
    }

    const DeviceValidationResult structureResult = validateSnapshot(next, nextConfigBlobs);
    if (!structureResult.ok()) {
        result.validation = structureResult;
        return result;
    }

    result.deviceId = deviceId;
    result.validation = reloadRuntimeFor(record, request.configBlob);
    if (!result.validation.ok()) {
        return result;
    }

    if (request.persistencePolicy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(request.persistencePolicy);
        if (!persistResult.ok()) {
            clearRuntime(deviceId);
            result.validation = persistResult;
            return result;
        }
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        ++registryRevision_;
        persistence_.markIndexDirty(now);
        persistence_.markConfigDirty(deviceId, now);
    }

    result.pendingPersistence = persistence_.hasPendingPersistence();
    syncRuntimeDependencyLinks(deviceId);
    auto* runtimePtr = this->runtime(deviceId);
    if (runtimePtr != nullptr) {
        if (request.enabled) {
            runtimePtr->begin(now);
        } else {
            runtimePtr->requestDisable();
        }
    }

    refreshDependentRuntimeStates(now);
    captureDirtyRuntimeRetainedStates(now);
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
    if (hasDuplicateNames(runtimes_, name, deviceId)) {
        result.validation = {DeviceError::InvalidConfig, "device name already exists"};
        return result;
    }

    IDeviceRuntime* currentRuntime = runtime(deviceId);
    if (currentRuntime == nullptr) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    DeviceRegistryEntry record = recordFromRuntime(*currentRuntime);
    DeviceConfigBlob oldConfigBlob{};
    DeviceConfigBlob configBlob{};
    if (!currentRuntime->serializeConfigBlob(oldConfigBlob) || !currentRuntime->serializeConfigBlob(configBlob)) {
        result.validation = {DeviceError::InvalidConfig, "device config is invalid"};
        return result;
    }
    DeviceBaseConfigV1 base{};
    base.enabled = currentRuntime->enabled() ? 1U : 0U;
    if (!copyBoundedText(base.name, currentRuntime->name())) {
        result.validation = {DeviceError::InvalidConfig, "device base config is invalid"};
        return result;
    }
    if (!copyBoundedText(base.name, name)) {
        result.validation = {DeviceError::BoundsExceeded, "device name exceeds supported length"};
        return result;
    }
    if (!currentRuntime->replaceBaseConfig(configBlob, base)) {
        result.validation = {DeviceError::StorageError, "failed to update device base config"};
        return result;
    }
    if (!currentRuntime->applyConfig(configBlob, now)) {
        result.validation = {DeviceError::InvalidConfig, "device config is invalid"};
        return result;
    }
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    const DeviceRegistryEntry oldRecord = recordFromRuntime(*currentRuntime);
    std::array<IDeviceRuntime*, kMaxDeviceDependencies> dependencyRuntimes{};
    const DeviceDependencyLink* dependencyLinks = currentRuntime->dependencyLinks();
    const uint8_t dependencyCount = currentRuntime->dependencyCount();
    for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
        dependencyRuntimes[index] = currentRuntime->dependencyRuntime(dependencyLinks[index].role);
        if (dependencyRuntimes[index] != nullptr) {
            dependencyRuntimes[index]->detachDependentRuntime(currentRuntime);
        }
    }
    currentRuntime->bindDeviceIdentity(record, configBlob);
    syncRuntimeDependencyLinks(deviceId);

    if (policy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(policy);
        if (!persistResult.ok()) {
            const DeviceDependencyLink* currentLinks = currentRuntime->dependencyLinks();
            const uint8_t currentDepCount = currentRuntime->dependencyCount();
            for (uint8_t index = 0; index < currentDepCount && currentLinks != nullptr; ++index) {
                if (IDeviceRuntime* dependency = currentRuntime->dependencyRuntime(currentLinks[index].role); dependency != nullptr) {
                    dependency->detachDependentRuntime(currentRuntime);
                }
            }
            (void)currentRuntime->applyConfig(oldConfigBlob, now);
            currentRuntime->bindDeviceIdentity(oldRecord, oldConfigBlob);
            for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
                if (dependencyRuntimes[index] != nullptr) {
                    currentRuntime->setDependencyRuntime(dependencyLinks[index].role, dependencyRuntimes[index]);
                    dependencyRuntimes[index]->attachDependentRuntime(currentRuntime);
                }
            }
            result.validation = persistResult;
            return result;
        }
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        ++registryRevision_;
        persistence_.markConfigDirty(deviceId, now);
    }

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.registryRevision = registryRevision_;
    updated.configRevision = record.header.configRevision;
    updated.deviceId = deviceId;
    updated.typeId = record.header.typeId;
    updated.status = effectiveStatus(deviceId);
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, "renamed");
    eventReporter_.emit(updated);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.configRevision = record.header.configRevision;
    accepted.deviceId = deviceId;
    accepted.typeId = record.header.typeId;
    accepted.status = updated.status;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, "rename");
    eventReporter_.emit(accepted);
    return result;
}

DeviceMutationResult DeviceRegistry::updateConfig(DeviceId deviceId, const BoundedBlob<kMaxDeviceConfigBytes>& configBlob,
                                                  uint32_t configVersion, uint32_t now, DevicePersistencePolicy policy) {
    return updateConfigAndDeps(deviceId, configBlob, configVersion, false, {}, 0, now, policy);
}

DeviceMutationResult DeviceRegistry::updateConfigAndDeps(DeviceId deviceId, const BoundedBlob<kMaxDeviceConfigBytes>& configBlob,
                                                         uint32_t configVersion, bool depsProvided,
                                                         const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                                         uint8_t depCount, uint32_t now, DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        result.validation = {DeviceError::BoundsExceeded, "device config exceeds supported size"};
        return result;
    }

    if (depsProvided && policy != DevicePersistencePolicy::Immediate) {
        result.validation = {DeviceError::InvalidConfig, "dependency reassignment requires immediate persistence"};
        return result;
    }

    IDeviceRuntime* currentRuntime = runtime(deviceId);
    if (currentRuntime == nullptr) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    const DeviceTypeDescriptor* descriptor = typeRegistry_.find(currentRuntime->typeId());
    if (descriptor == nullptr) {
        result.validation = {DeviceError::UnsupportedType, "unsupported device type"};
        return result;
    }

    DeviceRegistryEntry record = recordFromRuntime(*currentRuntime);
    const DeviceTypeId typeId = record.header.typeId;
    record.header.configVersion = configVersion != 0 ? configVersion : descriptor->currentConfigVersion;
    record.header.configRevision += 1;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    record.header.payloadChecksum = 0;
    if (depsProvided) {
        record.depCount = depCount;
        for (uint8_t index = 0; index < depCount && index < kMaxDeviceDependencies; ++index) {
            record.deps[index] = deps[index];
        }
    }
    const uint32_t nextConfigRevision = record.header.configRevision;

    DeviceRegistrySnapshot next{};
    DeviceConfigBlobMap nextConfigBlobs{};
    const DeviceValidationResult snapshotResult = buildSnapshot(next, nextConfigBlobs);
    if (!snapshotResult.ok()) {
        result.validation = snapshotResult;
        return result;
    }
    auto nextIt = std::find_if(next.records.begin(), next.records.end(),
                               [deviceId](const DeviceRegistryEntry& candidate) { return candidate.header.deviceId == deviceId; });
    if (nextIt == next.records.end()) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }
    *nextIt = record;
    nextConfigBlobs[deviceId] = configBlob;
    const DeviceValidationResult structureResult = validateSnapshot(next, nextConfigBlobs);
    if (!structureResult.ok()) {
        result.validation = structureResult;
        return result;
    }

    const bool dependenciesChanged = depsProvided && !sameDependencyLinks(currentRuntime->dependencyLinks(),
                                                                          currentRuntime->dependencyCount(), record.deps.data(), depCount);
    std::array<IDeviceRuntime*, kMaxDeviceDependencies> oldDependencyRuntimes{};
    const DeviceDependencyLink* oldDependencyLinks = currentRuntime->dependencyLinks();
    const uint8_t oldDependencyCount = currentRuntime->dependencyCount();
    if (dependenciesChanged) {
        for (uint8_t index = 0; index < oldDependencyCount && oldDependencyLinks != nullptr; ++index) {
            oldDependencyRuntimes[index] = currentRuntime->dependencyRuntime(oldDependencyLinks[index].role);
        }
    }
    const DeviceConfigUpdatePlan updatePlan = currentRuntime->planConfigUpdate(configBlob);
    if (updatePlan.endOldConfig) {
        currentRuntime->end(now);
    }
    if (!currentRuntime->applyConfig(configBlob, now)) {
        result.validation = {DeviceError::InvalidConfig, "device config is invalid"};
        return result;
    }
    currentRuntime->bindDeviceIdentity(record, configBlob);
    if (dependenciesChanged) {
        for (uint8_t index = 0; index < oldDependencyCount; ++index) {
            if (oldDependencyRuntimes[index] == nullptr) {
                continue;
            }
            oldDependencyRuntimes[index]->detachDependentRuntime(currentRuntime);
        }
    }
    syncRuntimeDependencyLinks(deviceId);
    if (updatePlan.resetStateMachine || dependenciesChanged) {
        currentRuntime->resetStateMachine(now);
    }
    for (const DeviceId childId : dependentDeviceIds(deviceId)) {
        syncRuntimeDependencyLinks(childId);
        if (auto* childRuntime = runtime(childId); childRuntime != nullptr) {
            childRuntime->requestReconfigure();
        }
    }

    if (policy == DevicePersistencePolicy::Immediate) {
        persistence_.markConfigDirty(deviceId, now);
        const DeviceValidationResult persistResult = persistIfNeeded(policy);
        if (!persistResult.ok()) {
            result.pendingPersistence = persistence_.hasPendingPersistence();
            result.validation = persistResult;
            return result;
        }
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        ++registryRevision_;
        persistence_.markConfigDirty(deviceId, now);
    }

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    refreshDependentRuntimeStates(now);
    captureDirtyRuntimeRetainedStates(now);
    emitRuntimeStatusChanges();

    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.registryRevision = registryRevision_;
    updated.configRevision = nextConfigRevision;
    updated.deviceId = deviceId;
    updated.typeId = typeId;
    updated.status = effectiveStatus(deviceId);
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, "config updated");
    eventReporter_.emit(updated);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.configRevision = nextConfigRevision;
    accepted.deviceId = deviceId;
    accepted.typeId = typeId;
    accepted.status = updated.status;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, "update_config");
    eventReporter_.emit(accepted);
    return result;
}

DeviceMutationResult DeviceRegistry::setDeps(DeviceId deviceId, const std::array<DeviceDependencyLink, kMaxDeviceDependencies>& deps,
                                             uint8_t depCount, uint32_t now, DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    IDeviceRuntime* currentRuntime = runtime(deviceId);
    if (currentRuntime == nullptr) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    if (policy != DevicePersistencePolicy::Immediate) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("setDeps rejected: non-immediate policy=%u", static_cast<unsigned>(policy));
        result.validation = {DeviceError::InvalidConfig, "dependency reassignment requires immediate persistence"};
        return result;
    }

    const uint8_t oldDepCount = currentRuntime->dependencyCount();
    const DeviceDependencyLink* oldDepLinks = currentRuntime->dependencyLinks();
    if (sameDependencyLinks(oldDepLinks, oldDepCount, deps.data(), depCount)) {
        result.pendingPersistence = persistence_.hasPendingPersistence();
        result.validation = {};
        return result;
    }

    DeviceConfigBlob configBlob{};
    if (!currentRuntime->serializeConfigBlob(configBlob)) {
        result.validation = {DeviceError::InvalidConfig, "device config is invalid"};
        return result;
    }
    DeviceRegistryEntry record = recordFromRuntime(*currentRuntime);
    record.depCount = depCount;
    for (uint8_t index = 0; index < depCount && index < kMaxDeviceDependencies; ++index) {
        record.deps[index] = deps[index];
    }

    DeviceRegistrySnapshot next{};
    DeviceConfigBlobMap nextConfigBlobs{};
    const DeviceValidationResult snapshotResult = buildSnapshot(next, nextConfigBlobs);
    if (!snapshotResult.ok()) {
        result.validation = snapshotResult;
        return result;
    }
    auto nextIt = std::find_if(next.records.begin(), next.records.end(),
                               [deviceId](const DeviceRegistryEntry& candidate) { return candidate.header.deviceId == deviceId; });
    if (nextIt == next.records.end()) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }
    *nextIt = record;
    const DeviceValidationResult structureResult = validateSnapshot(next, nextConfigBlobs);
    if (!structureResult.ok()) {
        result.validation = structureResult;
        return result;
    }

    std::array<IDeviceRuntime*, kMaxDeviceDependencies> oldDependencyRuntimes{};
    const DeviceDependencyLink* oldDependencyLinks = currentRuntime->dependencyLinks();
    for (uint8_t index = 0; index < oldDepCount && oldDependencyLinks != nullptr; ++index) {
        oldDependencyRuntimes[index] = currentRuntime->dependencyRuntime(oldDependencyLinks[index].role);
    }

    currentRuntime->bindDeviceIdentity(record, configBlob);
    for (uint8_t index = 0; index < oldDepCount; ++index) {
        if (oldDependencyRuntimes[index] == nullptr) {
            continue;
        }
        oldDependencyRuntimes[index]->detachDependentRuntime(currentRuntime);
    }
    syncRuntimeDependencyLinks(deviceId);
    currentRuntime->resetStateMachine(now);

    if (policy == DevicePersistencePolicy::Immediate) {
        persistence_.markConfigDirty(deviceId, now);
        const DeviceValidationResult persistResult = persistIfNeeded(policy);
        if (!persistResult.ok()) {
            result.pendingPersistence = persistence_.hasPendingPersistence();
            result.validation = persistResult;
            return result;
        }
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        ++registryRevision_;
        persistence_.markConfigDirty(deviceId, now);
    }

    refreshDependentRuntimeStates(now);
    captureDirtyRuntimeRetainedStates(now);
    emitRuntimeStatusChanges();

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.registryRevision = registryRevision_;
    updated.configRevision = record.header.configRevision;
    updated.deviceId = deviceId;
    updated.typeId = record.header.typeId;
    updated.status = effectiveStatus(deviceId);
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, "dependencies reassigned");
    eventReporter_.emit(updated);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.configRevision = record.header.configRevision;
    accepted.deviceId = deviceId;
    accepted.typeId = record.header.typeId;
    accepted.status = updated.status;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, "set_deps");
    eventReporter_.emit(accepted);
    EWFM_DEVICE_REGISTRY_LOG_INFO("dependencies reassigned device=%u depCount=%u", static_cast<unsigned>(deviceId),
                                  static_cast<unsigned>(depCount));
    return result;
}

DeviceMutationResult DeviceRegistry::setEnabled(DeviceId deviceId, bool enabled, uint32_t now, DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    IDeviceRuntime* currentRuntime = runtime(deviceId);
    if (currentRuntime == nullptr) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    DeviceConfigBlob oldConfigBlob{};
    DeviceConfigBlob configBlob{};
    if (!currentRuntime->serializeConfigBlob(oldConfigBlob) || !currentRuntime->serializeConfigBlob(configBlob)) {
        result.validation = {DeviceError::InvalidConfig, "device config is invalid"};
        return result;
    }
    DeviceRegistryEntry oldRecord = recordFromRuntime(*currentRuntime);
    DeviceRegistryEntry record = oldRecord;
    DeviceBaseConfigV1 base{};
    base.enabled = currentRuntime->enabled() ? 1U : 0U;
    if (!copyBoundedText(base.name, currentRuntime->name())) {
        result.validation = {DeviceError::InvalidConfig, "device base config is invalid"};
        return result;
    }
    base.enabled = enabled ? 1U : 0U;
    if (!currentRuntime->replaceBaseConfig(configBlob, base)) {
        result.validation = {DeviceError::StorageError, "failed to update device base config"};
        return result;
    }
    if (!currentRuntime->applyConfig(configBlob, now)) {
        result.validation = {DeviceError::InvalidConfig, "device config is invalid"};
        return result;
    }
    record.status = enabled ? DeviceStatus::Creating : DeviceStatus::Disabled;
    record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
    std::array<IDeviceRuntime*, kMaxDeviceDependencies> dependencyRuntimes{};
    const DeviceDependencyLink* dependencyLinks = currentRuntime->dependencyLinks();
    const uint8_t dependencyCount = currentRuntime->dependencyCount();
    for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
        dependencyRuntimes[index] = currentRuntime->dependencyRuntime(dependencyLinks[index].role);
        if (dependencyRuntimes[index] != nullptr) {
            dependencyRuntimes[index]->detachDependentRuntime(currentRuntime);
        }
    }
    currentRuntime->bindDeviceIdentity(record, configBlob);
    syncRuntimeDependencyLinks(deviceId);

    if (policy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(policy);
        if (!persistResult.ok()) {
            const DeviceDependencyLink* currentLinks = currentRuntime->dependencyLinks();
            const uint8_t currentDepCount = currentRuntime->dependencyCount();
            for (uint8_t index = 0; index < currentDepCount && currentLinks != nullptr; ++index) {
                if (IDeviceRuntime* dependency = currentRuntime->dependencyRuntime(currentLinks[index].role); dependency != nullptr) {
                    dependency->detachDependentRuntime(currentRuntime);
                }
            }
            (void)currentRuntime->applyConfig(oldConfigBlob, now);
            currentRuntime->bindDeviceIdentity(oldRecord, oldConfigBlob);
            for (uint8_t index = 0; index < dependencyCount && dependencyLinks != nullptr; ++index) {
                if (dependencyRuntimes[index] != nullptr) {
                    currentRuntime->setDependencyRuntime(dependencyLinks[index].role, dependencyRuntimes[index]);
                    dependencyRuntimes[index]->attachDependentRuntime(currentRuntime);
                }
            }
            result.validation = persistResult;
            return result;
        }
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
    } else {
        ++registryRevision_;
        persistence_.markConfigDirty(deviceId, now);
    }

    result.pendingPersistence = persistence_.hasPendingPersistence();
    result.validation = {};
    if (enabled) {
        currentRuntime->clearLifecycleRequests();
        currentRuntime->begin(now);
    } else {
        currentRuntime->clearLifecycleRequests();
        currentRuntime->requestDisable();
    }
    refreshDependentRuntimeStates(now);
    captureDirtyRuntimeRetainedStates(now);
    emitRuntimeStatusChanges();

    DeviceEvent updated{};
    updated.kind = DeviceEventKind::DeviceUpdated;
    updated.registryRevision = registryRevision_;
    updated.configRevision = record.header.configRevision;
    updated.deviceId = deviceId;
    updated.typeId = record.header.typeId;
    updated.status = effectiveStatus(deviceId);
    updated.pendingPersistence = persistence_.hasPendingPersistence();
    DeviceRegistryEventReporter::setEventDetail(updated, enabled ? "enabled" : "disabled");
    eventReporter_.emit(updated);

    DeviceEvent accepted{};
    accepted.kind = DeviceEventKind::CommandAccepted;
    accepted.registryRevision = registryRevision_;
    accepted.configRevision = record.header.configRevision;
    accepted.deviceId = deviceId;
    accepted.typeId = record.header.typeId;
    accepted.status = updated.status;
    accepted.pendingPersistence = persistence_.hasPendingPersistence();
    accepted.commandAccepted = true;
    DeviceRegistryEventReporter::setEventDetail(accepted, enabled ? "enable" : "disable");
    eventReporter_.emit(accepted);
    return result;
}

DeviceMutationResult DeviceRegistry::remove(DeviceId deviceId, uint32_t now, DevicePersistencePolicy policy) {
    DeviceMutationResult result{};
    if (runtime(deviceId) == nullptr) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    result.dependentDeviceIds = dependentDeviceIds(deviceId);
    if (!result.dependentDeviceIds.empty()) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("delete rejected: device=%u has %u dependents", static_cast<unsigned>(deviceId),
                                      static_cast<unsigned>(result.dependentDeviceIds.size()));
        result.validation = {DeviceError::InvalidRelationship, "device has dependent devices"};
        return result;
    }

    clearRuntime(deviceId);

    if (policy == DevicePersistencePolicy::Immediate) {
        const DeviceValidationResult persistResult = persistIfNeeded(policy);
        if (!persistResult.ok()) {
            result.validation = persistResult;
            return result;
        }
        ++registryRevision_;
        persistence_.clearConfigDirtyAfterImmediateFlush();
        (void)store_.removeRecord(deviceId);
    } else {
        ++registryRevision_;
        persistence_.markIndexDirty(now);
        persistence_.markConfigDirty(deviceId, now);
    }

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

    const IDeviceRuntime* runtimePtr = runtime(deviceId);
    if (runtimePtr == nullptr) {
        result.validation = {DeviceError::MissingRecord, "device not found"};
        return result;
    }

    const DeviceTypeDescriptor* descriptor = typeRegistry_.find(runtimePtr->typeId());
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
        updated.typeId = runtimePtr->typeId();
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
    updated.typeId = runtimePtr->typeId();
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
        return handleUpdateConfigCommand(*this, command, now);
    case DeviceCommandType::SetStatus:
    case DeviceCommandType::Scan:
    case DeviceCommandType::SetOutput:
    case DeviceCommandType::Custom: {
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
            rejected.typeId = runtime->typeId();
            rejected.commandAccepted = false;
            DeviceRegistryEventReporter::setEventDetail(rejected, result.validation.message);
            eventReporter_.emit(rejected);
            return result;
        }
        const DeviceValidationResult retainedResult = captureRuntimeRetainedState(command.deviceId, now);
        if (!retainedResult.ok()) {
            result.validation = retainedResult;
            return result;
        }
        refreshDependentRuntimeStates(now);
        emitRuntimeStatusChanges();
        DeviceEvent accepted{};
        accepted.kind = DeviceEventKind::CommandAccepted;
        accepted.registryRevision = registryRevision_;
        accepted.deviceId = command.deviceId;
        accepted.typeId = runtime->typeId();
        accepted.status = runtime->status();
        accepted.commandAccepted = true;
        accepted.pendingPersistence = persistence_.hasPendingPersistence();
        DeviceRegistryEventReporter::setEventDetail(accepted, "runtime command");
        eventReporter_.emit(accepted);
        result.pendingPersistence = persistence_.hasPendingPersistence();
        result.validation = {};
        return result;
    }
    case DeviceCommandType::SetDeps: {
        if (!command.depsPayloadValid) {
            DeviceMutationResult result{{DeviceError::InvalidCommand, "set_deps payload is missing"}, false};
            DeviceEvent rejected{};
            rejected.kind = DeviceEventKind::CommandRejected;
            rejected.registryRevision = registryRevision_;
            rejected.deviceId = command.deviceId;
            rejected.commandAccepted = false;
            DeviceRegistryEventReporter::setEventDetail(rejected, result.validation.message);
            eventReporter_.emit(rejected);
            return result;
        }
        return setDeps(command.deviceId, command.depsPayload.deps, command.depsPayload.depCount, now, command.persistencePolicy);
    }
    case DeviceCommandType::Create:
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

DeviceValidationResult DeviceRegistry::captureRuntimeRetainedState(DeviceId deviceId, uint32_t now) {
    IDeviceRuntime* runtimeInstance = runtime(deviceId);
    if (runtimeInstance == nullptr || !runtimeInstance->retainedStateDirty()) {
        return {};
    }

    RetainedStateRecord retained{};
    retained.deviceId = deviceId;
    if (!runtimeInstance->serializeRetainedState(retained)) {
        return {};
    }

    DeviceMutationResult result = setRetainedState(deviceId, retained.payload, now, DevicePersistencePolicy::Coalesced);
    if (!result.ok()) {
        return result.validation;
    }
    runtimeInstance->clearRetainedStateDirty();
    return {};
}

DeviceValidationResult DeviceRegistry::flushNow() {
    if (!persistence_.hasPendingPersistence()) {
        return {};
    }
    EWFM_DEVICE_REGISTRY_LOG_DEBUG("flush start: dirtyIndex=%d configDirty=%u retainedDirty=%u",
                                   static_cast<int>(persistence_.dirtyIndex()),
                                   static_cast<unsigned>(persistence_.dirtyConfigRecordIdsRef().size()),
                                   static_cast<unsigned>(persistence_.dirtyRetainedStateIdsRef().size()));

    const std::vector<DeviceId> dirtyConfigIds = persistence_.dirtyConfigRecordIdsRef();
    const std::vector<DeviceId> dirtyRetainedIds = persistence_.dirtyRetainedStateIdsRef();
    DeviceRegistrySnapshot snapshot{};
    DeviceConfigBlobMap configBlobs{};
    const DeviceValidationResult snapshotResult = buildSnapshot(snapshot, configBlobs);
    if (!snapshotResult.ok()) {
        return snapshotResult;
    }
    std::vector<DeviceId> staleRecordIds;
    staleRecordIds.reserve(dirtyConfigIds.size());
    for (const DeviceId deviceId : dirtyConfigIds) {
        if (runtime(deviceId) == nullptr) {
            staleRecordIds.push_back(deviceId);
        }
    }

    if (persistence_.hasConfigPersistenceWork()) {
        const DeviceValidationResult saveResult = store_.saveRecords(snapshot, configBlobs, dirtyConfigIds);
        if (!saveResult.ok()) {
            return saveResult;
        }
    }

    if (persistence_.dirtyIndex()) {
        const DeviceValidationResult indexResult = store_.saveIndex(snapshot);
        if (!indexResult.ok()) {
            return indexResult;
        }
    }

    if (persistence_.hasRetainedPersistenceWork()) {
        if (retainedStateStore_ == nullptr) {
            return {DeviceError::StorageError, "retained state store is unavailable"};
        }

        for (const DeviceId deviceId : dirtyRetainedIds) {
            const auto& pending = persistence_.pendingRetainedStateRecords();
            const auto pendingIt = pending.find(deviceId);
            if (pendingIt == pending.end()) {
                continue;
            }

            const DeviceValidationResult saveResult = retainedStateStore_->save(pendingIt->second);
            if (!saveResult.ok()) {
                return saveResult;
            }
        }
    }

    if (persistence_.dirtyIndex()) {
        for (const DeviceId staleRecordId : staleRecordIds) {
            (void)store_.removeRecord(staleRecordId);
        }
    }

    for (const DeviceId deviceId : dirtyConfigIds) {
        const IDeviceRuntime* runtimePtr = runtime(deviceId);
        if (runtimePtr == nullptr) {
            continue;
        }
        const DeviceRegistryEntry record = recordFromRuntime(*runtimePtr);
        DeviceEvent persisted{};
        persisted.kind = DeviceEventKind::ConfigPersisted;
        persisted.registryRevision = registryRevision_;
        persisted.configRevision = record.header.configRevision;
        persisted.deviceId = deviceId;
        persisted.typeId = record.header.typeId;
        persisted.status = effectiveStatusForRuntime(*runtimePtr);
        persisted.pendingPersistence = false;
        DeviceRegistryEventReporter::setEventDetail(persisted, "config persisted");
        eventReporter_.emit(persisted);
    }

    for (const DeviceId deviceId : dirtyRetainedIds) {
        const auto& pending = persistence_.pendingRetainedStateRecords();
        const auto pendingIt = pending.find(deviceId);
        if (pendingIt == pending.end()) {
            continue;
        }

        const IDeviceRuntime* runtimePtr = runtime(deviceId);
        if (runtimePtr == nullptr) {
            continue;
        }
        DeviceEvent persisted{};
        persisted.kind = DeviceEventKind::RetainedStateChanged;
        persisted.registryRevision = registryRevision_;
        persisted.deviceId = deviceId;
        persisted.typeId = runtimePtr->typeId();
        persisted.status = effectiveStatusForRuntime(*runtimePtr);
        persisted.pendingPersistence = false;
        DeviceRegistryEventReporter::setEventDetail(persisted, "retained state persisted");
        eventReporter_.emit(persisted);
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
    DeviceConfigBlobMap configBlobs{};
    for (const auto& entry : runtimes_) {
        if (entry.second.runtime == nullptr) {
            continue;
        }
        DeviceConfigBlob configBlob{};
        if (!entry.second.runtime->serializeConfigBlob(configBlob)) {
            return {DeviceError::InvalidConfig, "device config is invalid"};
        }
        configBlobs[entry.first] = configBlob;
    }
    return validateSnapshot(snapshot, configBlobs);
}

DeviceValidationResult DeviceRegistry::validateSnapshot(const DeviceRegistrySnapshot& snapshot,
                                                        const DeviceConfigBlobMap& configBlobs) const {
    if (snapshot.records.size() > kMaxDynamicDevices || snapshot.indexEntries.size() > kMaxDynamicDevices) {
        return {DeviceError::BoundsExceeded, "registry exceeds supported device count"};
    }

    if (!snapshot.indexEntries.empty() && snapshot.indexEntries.size() != snapshot.records.size()) {
        return {DeviceError::InvalidConfig, "index entry and record counts differ"};
    }

    std::map<DeviceId, const DeviceRegistryEntry*> recordsById;
    for (const auto& record : snapshot.records) {
        if (record.header.deviceId == 0) {
            return {DeviceError::InvalidDeviceId, "device id is invalid"};
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
        const auto configIt = configBlobs.find(record.header.deviceId);
        if (configIt == configBlobs.end()) {
            return {DeviceError::MissingRecord, "missing device config"};
        }
        const DeviceValidationResult recordResult = validateRecord(record, *descriptor, configIt->second);
        if (!recordResult.ok()) {
            return recordResult;
        }
        const DeviceValidationResult dependencyResult = validateDependencies(snapshot, record);
        if (!dependencyResult.ok()) {
            return dependencyResult;
        }
    }

    const DeviceValidationResult graphResult = validateAcyclicDependencyGraph(snapshot);
    if (!graphResult.ok()) {
        return graphResult;
    }

    return {};
}

DeviceValidationResult DeviceRegistry::validateRecord(const DeviceRegistryEntry& record, const DeviceTypeDescriptor& descriptor,
                                                      const DeviceConfigBlob& configBlob) const {
    if (record.header.configVersion == 0 || record.header.configVersion > descriptor.currentConfigVersion) {
        return {DeviceError::InvalidVersion, "unsupported config version"};
    }
    if (descriptor.validateConfig != nullptr) {
        return descriptor.validateConfig(record, configBlob);
    }
    return {};
}

DeviceValidationResult DeviceRegistry::validateDependencies(const DeviceRegistrySnapshot& snapshot,
                                                            const DeviceRegistryEntry& record) const {
    if (!record.hasDeps()) {
        return {};
    }

    const DeviceDependencyLink* links = record.dependencyLinks();
    const uint8_t depCount = record.dependencyCount();
    for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
        const DeviceDependencyLink& link = links[index];
        if (link.deviceId == 0) {
            return {DeviceError::InvalidRelationship, "dependency device id is missing"};
        }
        if (link.deviceId == record.header.deviceId) {
            return {DeviceError::InvalidRelationship, "self dependency relationship is not allowed"};
        }
        const auto dependencyIt = std::find_if(
            snapshot.records.begin(), snapshot.records.end(),
            [deviceId = link.deviceId](const DeviceRegistryEntry& candidate) { return candidate.header.deviceId == deviceId; });
        if (dependencyIt == snapshot.records.end()) {
            return {DeviceError::InvalidRelationship, "dependency device does not exist"};
        }
    }
    return {};
}

DeviceValidationResult DeviceRegistry::validateAcyclicDependencyGraph(const DeviceRegistrySnapshot& snapshot) const {
    std::map<DeviceId, const DeviceRegistryEntry*> recordsById;
    for (const auto& record : snapshot.records) {
        recordsById.emplace(record.header.deviceId, &record);
    }

    for (const auto& record : snapshot.records) {
        std::map<DeviceId, bool> seen;
        std::vector<DeviceId> stack;
        stack.push_back(record.header.deviceId);
        while (!stack.empty()) {
            const DeviceId currentId = stack.back();
            stack.pop_back();
            const auto currentIt = recordsById.find(currentId);
            if (currentIt == recordsById.end()) {
                continue;
            }
            const DeviceRegistryEntry& current = *currentIt->second;
            const DeviceDependencyLink* currentLinks = current.dependencyLinks();
            const uint8_t currentDepCount = current.dependencyCount();
            for (uint8_t index = 0; index < currentDepCount && currentLinks != nullptr; ++index) {
                const DeviceId dependencyId = currentLinks[index].deviceId;
                if (dependencyId == record.header.deviceId) {
                    return {DeviceError::InvalidRelationship, "cyclic dependency relationship detected"};
                }
                if (seen.find(dependencyId) != seen.end()) {
                    continue;
                }
                seen[dependencyId] = true;
                stack.push_back(dependencyId);
            }
        }
    }
    return {};
}

std::vector<DeviceId> DeviceRegistry::dependentDeviceIds(DeviceId deviceId) const {
    std::vector<DeviceId> dependents;
    for (const auto& entry : runtimes_) {
        const IDeviceRuntime* runtimePtr = entry.second.runtime.get();
        if (runtimePtr == nullptr) {
            continue;
        }
        const DeviceDependencyLink* links = runtimePtr->dependencyLinks();
        const uint8_t depCount = runtimePtr->dependencyCount();
        for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
            if (links[index].deviceId == deviceId) {
                dependents.push_back(runtimePtr->deviceId());
                break;
            }
        }
    }
    return dependents;
}

void DeviceRegistry::syncRuntimeDependencyLinks(DeviceId deviceId) {
    IDeviceRuntime* dependentRuntime = runtime(deviceId);
    if (dependentRuntime == nullptr) {
        return;
    }
    const DeviceDependencyLink* links = dependentRuntime->dependencyLinks();
    const uint8_t depCount = dependentRuntime->dependencyCount();
    for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
        if (IDeviceRuntime* dependencyRuntime = dependentRuntime->dependencyRuntime(links[index].role); dependencyRuntime != nullptr) {
            dependencyRuntime->detachDependentRuntime(dependentRuntime);
        }
        dependentRuntime->setDependencyRuntime(links[index].role, nullptr);
    }
    for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
        IDeviceRuntime* dependencyRuntime = runtime(links[index].deviceId);
        if (dependencyRuntime == nullptr) {
            continue;
        }
        dependentRuntime->setDependencyRuntime(links[index].role, dependencyRuntime);
        dependencyRuntime->attachDependentRuntime(dependentRuntime);
    }
}

DeviceStatus DeviceRegistry::effectiveStatusForRuntime(const IDeviceRuntime& runtime) const {
    if (!runtime.hasDependencies()) {
        return runtime.status();
    }
    const DeviceDependencyLink* links = runtime.dependencyLinks();
    const uint8_t depCount = runtime.dependencyCount();
    bool hasBlockedDependency = false;
    for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
        const IDeviceRuntime* dependency = this->runtime(links[index].deviceId);
        if (dependency == nullptr) {
            return DeviceStatus::DependencyBlocked;
        }
        const DeviceStatus dependencyStatus = effectiveStatusForRuntime(*dependency);
        if (dependencyStatus == DeviceStatus::Disabled) {
            return DeviceStatus::Disabled;
        }
        if (dependencyStatus != DeviceStatus::Ready) {
            hasBlockedDependency = true;
        }
    }
    if (hasBlockedDependency) {
        return DeviceStatus::DependencyBlocked;
    }
    return runtime.status();
}

void DeviceRegistry::refreshDependentRuntimeStates(uint32_t now) {
    (void)now;
    for (auto& entry : runtimes_) {
        IDeviceRuntime* runtimePtr = entry.second.runtime.get();
        if (runtimePtr == nullptr || !runtimePtr->hasDependencies()) {
            continue;
        }
        const DeviceDependencyLink* links = runtimePtr->dependencyLinks();
        const uint8_t depCount = runtimePtr->dependencyCount();
        bool shouldDisable = false;
        bool shouldReconfigure = false;
        for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
            const IDeviceRuntime* dependency = runtime(links[index].deviceId);
            if (dependency == nullptr || dependency->status() == DeviceStatus::Faulted || dependency->status() == DeviceStatus::Deleting) {
                shouldReconfigure = true;
                continue;
            }
            if (dependency->status() == DeviceStatus::Disabled) {
                shouldDisable = true;
            } else if (dependency->status() != DeviceStatus::Ready) {
                shouldReconfigure = true;
            }
        }
        if (shouldDisable && runtimePtr->status() != DeviceStatus::Disabled) {
            runtimePtr->requestDisable();
        } else if (shouldReconfigure && runtimePtr->status() != DeviceStatus::Reconfiguring) {
            runtimePtr->requestReconfigure();
        }
    }
}

void DeviceRegistry::captureDirtyRuntimeRetainedStates(uint32_t now) {
    for (const auto& entry : runtimes_) {
        if (entry.second.runtime == nullptr || !entry.second.runtime->retainedStateDirty()) {
            continue;
        }
        const DeviceValidationResult retainedResult = captureRuntimeRetainedState(entry.first, now);
        if (!retainedResult.ok()) {
            EWFM_DEVICE_REGISTRY_LOG_WARN("retained capture failed for device=%u: %s", static_cast<unsigned>(entry.first),
                                          retainedResult.message);
        }
    }
}

void DeviceRegistry::emitRuntimeStatusChanges() {
    for (const auto& entry : runtimes_) {
        if (entry.second.runtime == nullptr) {
            continue;
        }
        const DeviceStatus current = entry.second.runtime->status();
        const DeviceRegistryEntry record = recordFromRuntime(*entry.second.runtime);
        const DeviceTypeId typeId = record.header.typeId;
        eventReporter_.emitRuntimeStatusChangeIfNeeded(entry.first, typeId, current, registryRevision_,
                                                       persistence_.hasPendingPersistence(), "runtime status changed");
        if (entry.second.runtime->runtimeStateDirty()) {
            DeviceEvent stateChanged{};
            stateChanged.kind = DeviceEventKind::StateChanged;
            stateChanged.registryRevision = registryRevision_;
            stateChanged.configRevision = record.header.configRevision;
            stateChanged.deviceId = entry.first;
            stateChanged.typeId = typeId;
            stateChanged.status = current;
            stateChanged.pendingPersistence = persistence_.hasPendingPersistence();
            DeviceRegistryEventReporter::setEventDetail(stateChanged, "runtime state changed");
            eventReporter_.emit(stateChanged);
            entry.second.runtime->clearRuntimeStateDirty();
        }
    }
}

DeviceValidationResult DeviceRegistry::persistIfNeeded(DevicePersistencePolicy policy) {
    if (policy == DevicePersistencePolicy::Immediate) {
        DeviceRegistrySnapshot snapshot{};
        DeviceConfigBlobMap configBlobs{};
        const DeviceValidationResult snapshotResult = buildSnapshot(snapshot, configBlobs);
        if (!snapshotResult.ok()) {
            return snapshotResult;
        }
        const DeviceValidationResult saveResult = store_.save(snapshot, configBlobs);
        if (!saveResult.ok()) {
            return saveResult;
        }
        return {};
    }
    return {};
}

DeviceValidationResult DeviceRegistry::reloadRuntimeFor(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    const DeviceTypeDescriptor* descriptor = typeRegistry_.find(record.header.typeId);
    if (descriptor == nullptr) {
        return {DeviceError::UnsupportedType, "unsupported device type"};
    }
    if (descriptor->createRuntime == nullptr) {
        return {};
    }
    if (descriptor->validateConfig != nullptr) {
        const DeviceValidationResult validation = descriptor->validateConfig(record, configBlob);
        if (!validation.ok()) {
            return validation;
        }
    }

    DeviceRuntimeSlot entry;
    entry.descriptor = descriptor;
    entry.runtime = descriptor->createRuntime(record, configBlob);
    if (entry.runtime == nullptr) {
        return {DeviceError::StorageError, "failed to create runtime"};
    }
    entry.runtime->bindDeviceIdentity(record, configBlob);
    if (descriptor->supportsRetainedState && retainedStateStore_ != nullptr) {
        RetainedStateRecord retained{};
        const DeviceValidationResult retainedResult = retainedStateStore_->load(record.header.deviceId, retained);
        if (retainedResult.ok()) {
            (void)entry.runtime->applyRetainedStateRecord(retained);
        } else if (retainedResult.error != DeviceError::MissingRecord) {
            return retainedResult;
        }
    }
    runtimes_[record.header.deviceId] = std::move(entry);
    return {};
}

DeviceValidationResult DeviceRegistry::replaceRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    clearRuntime(record.header.deviceId);
    return reloadRuntimeFor(record, configBlob);
}

DeviceValidationResult DeviceRegistry::buildSnapshot(DeviceRegistrySnapshot& snapshot, DeviceConfigBlobMap& configBlobs) const {
    snapshot = {};
    configBlobs.clear();
    for (const auto& entry : runtimes_) {
        const IDeviceRuntime* runtimePtr = entry.second.runtime.get();
        if (runtimePtr == nullptr) {
            continue;
        }
        DeviceConfigBlob configBlob{};
        if (!runtimePtr->serializeConfigBlob(configBlob)) {
            return {DeviceError::InvalidConfig, "device config is invalid"};
        }
        DeviceRegistryEntry record = recordFromRuntime(*runtimePtr);
        record.header.payloadLength = static_cast<uint32_t>(configBlob.size());
        snapshot.records.push_back(record);
        snapshot.indexEntries.push_back({record.header.deviceId, record.header.typeId});
        configBlobs[record.header.deviceId] = configBlob;
    }
    return {};
}

void DeviceRegistry::clearRuntime(DeviceId deviceId) {
    const auto it = runtimes_.find(deviceId);
    if (it != runtimes_.end() && it->second.runtime != nullptr) {
        const IDeviceRuntime* runtimePtr = it->second.runtime.get();
        const DeviceDependencyLink* links = runtimePtr->dependencyLinks();
        const uint8_t depCount = runtimePtr->dependencyCount();
        for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
            if (IDeviceRuntime* dependency = runtime(links[index].deviceId); dependency != nullptr) {
                dependency->detachDependentRuntime(it->second.runtime.get());
            }
        }
        const std::vector<IDeviceRuntime*> dependents = it->second.runtime->dependentRuntimes();
        for (IDeviceRuntime* dependent : dependents) {
            if (dependent == nullptr) {
                continue;
            }
            const DeviceDependencyLink* dependentLinks = dependent->dependencyLinks();
            const uint8_t dependentDepCount = dependent->dependencyCount();
            for (uint8_t index = 0; index < dependentDepCount && dependentLinks != nullptr; ++index) {
                if (dependentLinks[index].deviceId == deviceId) {
                    dependent->setDependencyRuntime(dependentLinks[index].role, nullptr);
                }
            }
        }
    }
    runtimes_.erase(deviceId);
    eventReporter_.clearRuntimeStatus(deviceId);
}

} // namespace ewfm
