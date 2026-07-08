#include "devices/registry/DeviceRegistrySnapshotValidator.h"

#include "devices/registry/DeviceRegistryBinaryCodec.h"

#include <algorithm>
#include <map>

namespace ewfm {

namespace {
bool hasDuplicateEntries(const std::vector<DeviceIndexEntry>& entries) {
    std::map<DeviceId, DeviceTypeId> seen;
    for (const auto& entry : entries) {
        if (entry.deviceId == 0) {
            return true;
        }
        if (seen.find(entry.deviceId) != seen.end()) {
            return true;
        }
        seen.emplace(entry.deviceId, entry.typeId);
    }
    return false;
}

bool recordDependsOn(const DeviceRegistryEntry& record, DeviceId deviceId) {
    const DeviceDependencyLink* links = record.dependencyLinks();
    const uint8_t depCount = record.dependencyCount();
    for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
        if (links[index].deviceId == deviceId) {
            return true;
        }
    }
    return false;
}

DeviceValidationResult validateDependencyGraph(const std::map<DeviceId, const DeviceRegistryEntry*>& recordsById,
                                               const DeviceRegistryEntry& record, std::map<DeviceId, bool>& seen) {
    if (seen.find(record.header.deviceId) != seen.end()) {
        return {DeviceError::InvalidRelationship, "cyclic dependency relationship detected"};
    }
    seen[record.header.deviceId] = true;
    const DeviceDependencyLink* links = record.dependencyLinks();
    const uint8_t depCount = record.dependencyCount();
    for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
        const DeviceDependencyLink& link = links[index];
        if (link.deviceId == record.header.deviceId) {
            return {DeviceError::InvalidRelationship, "self dependency relationship is not allowed"};
        }
        const auto depIt = recordsById.find(link.deviceId);
        if (depIt == recordsById.end()) {
            return {DeviceError::InvalidRelationship, "dependency device does not exist"};
        }
        const DeviceRegistryEntry& dependencyRecord = *depIt->second;
        if (recordDependsOn(dependencyRecord, record.header.deviceId)) {
            return {DeviceError::InvalidRelationship, "cyclic dependency relationship detected"};
        }
        auto nextSeen = seen;
        const DeviceValidationResult nestedResult = validateDependencyGraph(recordsById, dependencyRecord, nextSeen);
        if (!nestedResult.ok()) {
            return nestedResult;
        }
    }
    return {};
}
} // namespace

DeviceValidationResult DeviceRegistrySnapshotValidator::validateStructure(const DeviceRegistrySnapshot& snapshot) {
    if (snapshot.indexEntries.size() > kMaxDynamicDevices || snapshot.records.size() > kMaxDynamicDevices) {
        return {DeviceError::BoundsExceeded, "registry exceeds supported device count"};
    }

    if (snapshot.indexEntries.empty() && snapshot.records.empty()) {
        return {};
    }

    if (snapshot.indexEntries.empty() && !snapshot.records.empty()) {
        return {DeviceError::InvalidConfig, "registry snapshot is missing index entries"};
    }

    if (snapshot.indexEntries.size() != snapshot.records.size()) {
        return {DeviceError::InvalidConfig, "index entry and record counts differ"};
    }

    if (hasDuplicateEntries(snapshot.indexEntries)) {
        return {DeviceError::DuplicateDeviceId, "duplicate device ids found in registry snapshot"};
    }

    std::map<DeviceId, const DeviceRegistryEntry*> recordById;
    for (const auto& record : snapshot.records) {
        if (record.header.deviceId == 0 || record.header.typeId == 0) {
            return {DeviceError::InvalidDeviceId, "device id or type id is invalid"};
        }
        if (record.header.recordVersion != 0 && record.header.recordVersion != kDeviceRecordHeaderVersion) {
            return {DeviceError::InvalidVersion, "unsupported device record version"};
        }
        if (record.header.configVersion == 0) {
            return {DeviceError::InvalidVersion, "missing config version"};
        }
        if (static_cast<uint8_t>(record.status) > static_cast<uint8_t>(DeviceStatus::Deleting) ||
            static_cast<uint8_t>(record.persistencePolicy) > static_cast<uint8_t>(DevicePersistencePolicy::Coalesced)) {
            return {DeviceError::CorruptRecord, "device record contains invalid status or persistence policy"};
        }
        if (record.dependencyCount() > kMaxDeviceDependencies) {
            return {DeviceError::BoundsExceeded, "device record exceeds supported dependency count"};
        }
        const DeviceDependencyLink* links = record.dependencyLinks();
        for (uint8_t index = 0; index < record.dependencyCount() && links != nullptr; ++index) {
            if (links[index].role == DeviceRole::Unknown) {
                return {DeviceError::InvalidRelationship, "duplicate dependency role or invalid dependency role"};
            }
        }
        if (recordById.find(record.header.deviceId) != recordById.end()) {
            return {DeviceError::DuplicateDeviceId, "duplicate record device id"};
        }
        recordById.emplace(record.header.deviceId, &record);
    }

    for (const auto& entry : snapshot.indexEntries) {
        const auto it = recordById.find(entry.deviceId);
        if (it == recordById.end()) {
            return {DeviceError::MissingRecord, "missing device record for index entry"};
        }
        const DeviceRegistryEntry& record = *it->second;
        if (record.header.typeId != entry.typeId) {
            return {DeviceError::InvalidConfig, "index entry type does not match device record"};
        }
    }

    for (const auto& record : snapshot.records) {
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
            if (recordById.find(link.deviceId) == recordById.end()) {
                return {DeviceError::InvalidRelationship, "dependency device does not exist"};
            }
        }
    }

    for (const auto& record : snapshot.records) {
        std::map<DeviceId, bool> seen;
        const DeviceValidationResult graphResult = validateDependencyGraph(recordById, record, seen);
        if (!graphResult.ok()) {
            return graphResult;
        }
    }

    return {};
}

DeviceValidationResult DeviceRegistrySnapshotValidator::validateTypedRelationships(const DeviceRegistrySnapshot& snapshot,
                                                                                   const DeviceTypeRegistry* typeRegistry) {
    if (typeRegistry == nullptr) {
        return {};
    }

    std::map<DeviceId, const DeviceRegistryEntry*> recordsById;
    for (const auto& record : snapshot.records) {
        recordsById.emplace(record.header.deviceId, &record);
    }

    std::map<DeviceId, size_t> dependentCounts;
    for (const auto& record : snapshot.records) {
        const DeviceTypeDescriptor* dependentDescriptor = typeRegistry->find(record.header.typeId);
        if (dependentDescriptor == nullptr) {
            return {DeviceError::UnsupportedType, "unsupported device type"};
        }
        if (record.header.configVersion > dependentDescriptor->currentConfigVersion) {
            return {DeviceError::InvalidVersion, "unsupported device config version"};
        }

        const DeviceDependencyLink* links = record.dependencyLinks();
        const uint8_t depCount = record.dependencyCount();
        for (uint8_t index = 0; index < depCount && links != nullptr; ++index) {
            const DeviceDependencyLink& link = links[index];
            const auto depIt = recordsById.find(link.deviceId);
            if (depIt == recordsById.end()) {
                return {DeviceError::InvalidRelationship, "missing dependency record"};
            }

            const DeviceRegistryEntry& dependencyRecord = *depIt->second;
            const DeviceTypeDescriptor* dependencyDescriptor = typeRegistry->find(dependencyRecord.header.typeId);
            if (dependencyDescriptor == nullptr) {
                return {DeviceError::UnsupportedType, "unsupported device type"};
            }

            const auto requirementIt =
                std::find_if(dependentDescriptor->dependencyRequirements.begin(), dependentDescriptor->dependencyRequirements.end(),
                             [role = link.role](const DeviceDependencyRequirement& requirement) { return requirement.role == role; });
            if (requirementIt == dependentDescriptor->dependencyRequirements.end()) {
                return {DeviceError::InvalidRelationship, "unsupported dependency role"};
            }
            if (dependencyDescriptor->providedRole != requirementIt->role) {
                return {DeviceError::InvalidRelationship, "incompatible dependency type"};
            }
            const size_t nextCount = ++dependentCounts[dependencyRecord.header.deviceId];
            if (dependencyDescriptor->maxDependents > 0 && nextCount > dependencyDescriptor->maxDependents) {
                return {DeviceError::InvalidRelationship, "dependency dependent limit exceeded"};
            }
        }

        for (const auto& requirement : dependentDescriptor->dependencyRequirements) {
            if (!requirement.required) {
                continue;
            }
            const DeviceDependencyLink* links = record.dependencyLinks();
            const uint8_t depCount = record.dependencyCount();
            const bool hasRole = std::find_if(links, links + depCount, [&requirement](const DeviceDependencyLink& link) {
                                     return link.role == requirement.role;
                                 }) != links + depCount;
            if (!hasRole) {
                return {DeviceError::InvalidRelationship, "required dependency role is missing"};
            }
        }
    }

    return {};
}

} // namespace ewfm
