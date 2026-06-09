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

    std::map<DeviceId, const DeviceRecord*> recordById;
    for (const auto& record : snapshot.records) {
        if (record.header.deviceId == 0 || record.header.typeId == 0) {
            return {DeviceError::InvalidDeviceId, "device id or type id is invalid"};
        }
        if (record.name.size() > kMaxDynamicDeviceNameLength) {
            return {DeviceError::BoundsExceeded, "device name exceeds supported length"};
        }
        if (record.configPayload.size() > kMaxDeviceConfigBytes) {
            return {DeviceError::BoundsExceeded, "device config exceeds supported size"};
        }
        if (record.header.payloadLength != 0 && record.header.payloadLength != record.configPayload.size()) {
            return {DeviceError::InvalidConfig, "payload length does not match payload"};
        }
        if (record.header.payloadChecksum != 0 &&
            record.header.payloadChecksum != DeviceRegistryBinaryCodec::payloadChecksum(record.configPayload)) {
            return {DeviceError::InvalidConfig, "payload checksum does not match payload"};
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
        if (!record.hasParent && record.parentDeviceId != 0) {
            return {DeviceError::InvalidRelationship, "device record contains unexpected parent reference"};
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
        const DeviceRecord& record = *it->second;
        if (record.header.typeId != entry.typeId) {
            return {DeviceError::InvalidConfig, "index entry type does not match device record"};
        }
    }

    for (const auto& record : snapshot.records) {
        if (!record.hasParent) {
            continue;
        }
        if (record.parentDeviceId == 0) {
            return {DeviceError::InvalidRelationship, "parent device id is missing"};
        }
        if (record.parentDeviceId == record.header.deviceId) {
            return {DeviceError::InvalidRelationship, "self parent relationship is not allowed"};
        }
        if (recordById.find(record.parentDeviceId) == recordById.end()) {
            return {DeviceError::InvalidRelationship, "parent device id is missing"};
        }
    }

    for (const auto& entry : snapshot.indexEntries) {
        const auto childIt = recordById.find(entry.deviceId);
        if (childIt == recordById.end()) {
            continue;
        }
        const DeviceRecord& child = *childIt->second;
        if (!child.hasParent) {
            continue;
        }

        DeviceId parentId = child.parentDeviceId;
        std::map<DeviceId, bool> seen;
        while (parentId != 0) {
            if (seen.find(parentId) != seen.end()) {
                return {DeviceError::InvalidRelationship, "cyclic parent relationship detected"};
            }
            seen[parentId] = true;
            const auto current = recordById.find(parentId);
            if (current == recordById.end()) {
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

DeviceValidationResult DeviceRegistrySnapshotValidator::validateTypedRelationships(const DeviceRegistrySnapshot& snapshot,
                                                                                   const DeviceTypeRegistry* typeRegistry) {
    if (typeRegistry == nullptr) {
        return {};
    }

    std::map<DeviceId, const DeviceRecord*> recordsById;
    for (const auto& record : snapshot.records) {
        recordsById.emplace(record.header.deviceId, &record);
    }

    std::map<DeviceId, size_t> childCounts;
    for (const auto& entry : snapshot.indexEntries) {
        const auto childIt = recordsById.find(entry.deviceId);
        if (childIt == recordsById.end()) {
            return {DeviceError::MissingRecord, "missing device record for index entry"};
        }

        const DeviceRecord& childRecord = *childIt->second;
        const DeviceTypeDescriptor* childDescriptor = typeRegistry->find(childRecord.header.typeId);
        if (childDescriptor == nullptr) {
            return {DeviceError::UnsupportedType, "unsupported device type"};
        }
        if (childRecord.header.configVersion > childDescriptor->currentConfigVersion) {
            return {DeviceError::InvalidVersion, "unsupported device config version"};
        }

        if (!childRecord.hasParent) {
            continue;
        }

        const auto parentIt = recordsById.find(childRecord.parentDeviceId);
        if (parentIt == recordsById.end()) {
            return {DeviceError::InvalidRelationship, "missing parent record"};
        }

        const DeviceRecord& parentRecord = *parentIt->second;
        const DeviceTypeDescriptor* parentDescriptor = typeRegistry->find(parentRecord.header.typeId);
        if (parentDescriptor == nullptr) {
            return {DeviceError::UnsupportedType, "unsupported device type"};
        }

        if (!childDescriptor->compatibleParentTypes.empty()) {
            const bool parentCompatible =
                std::find(childDescriptor->compatibleParentTypes.begin(), childDescriptor->compatibleParentTypes.end(),
                          parentRecord.header.typeId) != childDescriptor->compatibleParentTypes.end();
            if (!parentCompatible) {
                return {DeviceError::InvalidRelationship, "incompatible parent type"};
            }
        }

        if (!parentDescriptor->canHaveChildren) {
            return {DeviceError::InvalidRelationship, "parent type cannot have children"};
        }

        const size_t nextChildCount = ++childCounts[childRecord.parentDeviceId];
        if (parentDescriptor->maxChildren > 0 && nextChildCount > parentDescriptor->maxChildren) {
            return {DeviceError::InvalidRelationship, "parent child limit exceeded"};
        }
    }

    return {};
}

} // namespace ewfm
