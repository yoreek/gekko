#include "devices/registry/DeviceRegistry.h"

#include <algorithm>

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

} // namespace

DeviceRegistry::DeviceRegistry(DeviceRegistryStore& store, const DeviceTypeRegistry& typeRegistry, IDeviceIdSource& idSource)
    : store_(store), typeRegistry_(typeRegistry), idSource_(idSource) {}

DeviceValidationResult DeviceRegistry::begin(uint32_t now) {
    snapshot_ = {};
    runtimes_.clear();
    registryRevision_ = 0;
    dirty_ = false;
    dirtySince_ = now;
    lastMutationAt_ = now;

    const DeviceValidationResult loadResult = store_.load(snapshot_, &typeRegistry_);
    if (!loadResult.ok()) {
        snapshot_ = {};
        return loadResult;
    }

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
        if (record.enabled) {
            const DeviceValidationResult runtimeResult = reloadRuntimeFor(record.header.deviceId);
            if (!runtimeResult.ok()) {
                return runtimeResult;
            }
            if (auto* runtime = this->runtime(record.header.deviceId); runtime != nullptr) {
                runtime->begin(now);
            }
        }
    }

    return {};
}

void DeviceRegistry::tick(uint32_t now) {
    if (!dirty_) {
        return;
    }

    if (firstDirtyAt_ == kDirtyTimestampUnset) {
        firstDirtyAt_ = now;
        dirtySince_ = now;
        return;
    }

    const uint32_t elapsed = static_cast<uint32_t>(now - dirtySince_);
    const uint32_t dirtyAge = static_cast<uint32_t>(now - firstDirtyAt_);
    if (elapsed < kPersistenceDebounceMs && dirtyAge < kPersistenceMaxDelayMs) {
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
}

void DeviceRegistry::tick100ms(uint32_t now) {
    for (auto& entry : runtimes_) {
        if (entry.second.runtime != nullptr && entry.second.descriptor != nullptr && entry.second.descriptor->ticks100ms) {
            entry.second.runtime->tick100ms(now);
        }
    }
}

void DeviceRegistry::tick1s(uint32_t now) {
    for (auto& entry : runtimes_) {
        if (entry.second.runtime != nullptr && entry.second.descriptor != nullptr && entry.second.descriptor->ticks1s) {
            entry.second.runtime->tick1s(now);
        }
    }
}

const DeviceRegistrySnapshot& DeviceRegistry::snapshot() const {
    return snapshot_;
}

uint32_t DeviceRegistry::registryRevision() const {
    return registryRevision_;
}

bool DeviceRegistry::hasPendingPersistence() const {
    return dirty_;
}

std::vector<DeviceRecord> DeviceRegistry::list() const {
    return snapshot_.records;
}

const DeviceRecord* DeviceRegistry::find(DeviceId deviceId) const {
    for (const auto& record : snapshot_.records) {
        if (record.header.deviceId == deviceId) {
            return &record;
        }
    }
    return nullptr;
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
        result.validation = {DeviceError::UnsupportedType, "unsupported device type"};
        return result;
    }

    if (request.name.empty()) {
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
        markClean();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        markDirty(now);
    }

    result.pendingPersistence = dirty_;
    if (record.enabled) {
        result.validation = reloadRuntimeFor(deviceId);
        if (!result.validation.ok()) {
            return result;
        }
        auto* runtimePtr = this->runtime(deviceId);
        if (runtimePtr != nullptr) {
            runtimePtr->begin(now);
        }
    }

    result.validation = {};
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
        markClean();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        markDirty(now);
    }

    result.pendingPersistence = dirty_;
    result.validation = {};
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
        markClean();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        markDirty(now);
    }

    result.pendingPersistence = dirty_;
    result.validation = {};
    if (const auto runtimeIt = runtimes_.find(deviceId); runtimeIt != runtimes_.end() && runtimeIt->second.runtime != nullptr) {
        runtimeIt->second.runtime->requestReconfigure();
    }
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
        markClean();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        markDirty(now);
    }

    result.pendingPersistence = dirty_;
    result.validation = {};
    if (enabled) {
        result.validation = reloadRuntimeFor(deviceId);
        if (!result.validation.ok()) {
            return result;
        }
        const auto runtimePtr = runtime(deviceId);
        if (runtimePtr != nullptr) {
            runtimePtr->begin(now);
        }
    } else {
        clearRuntime(deviceId);
    }
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
        markClean();
    } else {
        snapshot_ = std::move(next);
        ++registryRevision_;
        markDirty(now);
    }

    clearRuntime(deviceId);
    result.pendingPersistence = dirty_;
    result.validation = {};
    return result;
}

DeviceMutationResult DeviceRegistry::command(const DeviceCommand& command, uint32_t now) {
    switch (command.type) {
    case DeviceCommandType::Rename:
        return rename(command.deviceId, command.payload, now, command.persistencePolicy);
    case DeviceCommandType::Enable:
        return setEnabled(command.deviceId, true, now, command.persistencePolicy);
    case DeviceCommandType::Disable:
        return setEnabled(command.deviceId, false, now, command.persistencePolicy);
    case DeviceCommandType::Delete:
        return remove(command.deviceId, now, command.persistencePolicy);
    case DeviceCommandType::UpdateConfig:
        return updateConfig(command.deviceId, command.payload, 0, now, command.persistencePolicy);
    case DeviceCommandType::SetStatus: {
        DeviceMutationResult result{};
        auto runtime = this->runtime(command.deviceId);
        if (runtime == nullptr) {
            result.validation = {DeviceError::MissingRecord, "device runtime not available"};
            return result;
        }
        if (!runtime->handleCommand(command)) {
            result.validation = {DeviceError::InvalidCommand, "command rejected by runtime"};
            return result;
        }
        result.validation = {};
        return result;
    }
    case DeviceCommandType::Create:
    case DeviceCommandType::Custom:
    case DeviceCommandType::None:
        break;
    }

    return {{DeviceError::InvalidCommand, "unsupported command"}, false};
}

DeviceValidationResult DeviceRegistry::flushNow() {
    if (!dirty_) {
        return {};
    }

    const DeviceValidationResult saveResult = store_.save(snapshot_);
    if (!saveResult.ok()) {
        return saveResult;
    }

    markClean();
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

    RuntimeEntry entry;
    entry.descriptor = descriptor;
    entry.runtime = descriptor->createRuntime(*record);
    if (entry.runtime == nullptr) {
        return {DeviceError::StorageError, "failed to create runtime"};
    }
    runtimes_[deviceId] = std::move(entry);
    return {};
}

void DeviceRegistry::clearRuntime(DeviceId deviceId) {
    runtimes_.erase(deviceId);
}

void DeviceRegistry::clearRuntimeIfDisabled(DeviceId deviceId) {
    const DeviceRecord* record = find(deviceId);
    if (record != nullptr && !record->enabled) {
        clearRuntime(deviceId);
    }
}

void DeviceRegistry::markDirty(uint32_t now) {
    if (!dirty_) {
        firstDirtyAt_ = now;
    }
    dirty_ = true;
    dirtySince_ = now;
    lastMutationAt_ = now;
}

void DeviceRegistry::markClean() {
    dirty_ = false;
    firstDirtyAt_ = kDirtyTimestampUnset;
    dirtySince_ = kDirtyTimestampUnset;
}

} // namespace ewfm
