#include "devices/registry/DeviceRegistryStore.h"

#include "devices/registry/DeviceRegistryBinaryCodec.h"
#include "devices/registry/DeviceRegistrySnapshotValidator.h"

#include <map>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device_registry";
constexpr const char* kIndexKey = "index";
} // namespace

DeviceRegistryStore::DeviceRegistryStore(IConfigStorage& storage) : storage_(storage) {}

bool DeviceRegistryStore::begin(bool readOnly) {
    return storage_.begin(kNamespace, readOnly);
}

DeviceValidationResult DeviceRegistryStore::load(DeviceRegistrySnapshot& snapshot, const DeviceTypeRegistry* typeRegistry) {
    snapshot = {};

    if (!storage_.hasKey(kIndexKey)) {
        return {};
    }

    std::string indexBlob;
    if (!storage_.getString(kIndexKey, indexBlob)) {
        return {DeviceError::StorageError, "failed to read registry index"};
    }
    std::string indexBytes;
    if (!DeviceRegistryBinaryCodec::fromHex(indexBlob, indexBytes)) {
        return {DeviceError::CorruptRecord, "registry index is not valid hex"};
    }
    indexBlob = std::move(indexBytes);

    DeviceValidationResult indexResult = DeviceRegistryBinaryCodec::parseIndex(indexBlob, snapshot);
    if (!indexResult.ok()) {
        snapshot = {};
        return indexResult;
    }

    std::map<DeviceId, DeviceRecord> recordsById;
    for (const auto& entry : snapshot.indexEntries) {
        const std::string recordKey = DeviceRegistryBinaryCodec::makeRecordKey(entry.deviceId);
        std::string recordBlob;
        if (!storage_.getString(recordKey.c_str(), recordBlob)) {
            snapshot = {};
            return {DeviceError::MissingRecord, "missing device record"};
        }
        std::string recordBytes;
        if (!DeviceRegistryBinaryCodec::fromHex(recordBlob, recordBytes)) {
            snapshot = {};
            return {DeviceError::CorruptRecord, "device record is not valid hex"};
        }
        recordBlob = std::move(recordBytes);

        DeviceRecord record{};
        DeviceValidationResult recordResult = DeviceRegistryBinaryCodec::parseRecord(recordBlob, record);
        if (!recordResult.ok()) {
            snapshot = {};
            return recordResult;
        }

        if (record.header.deviceId != entry.deviceId || record.header.typeId != entry.typeId) {
            snapshot = {};
            return {DeviceError::InvalidConfig, "index entry does not match device record"};
        }

        if (typeRegistry != nullptr) {
            const DeviceTypeDescriptor* descriptor = typeRegistry->find(record.header.typeId);
            if (descriptor == nullptr) {
                snapshot = {};
                return {DeviceError::UnsupportedType, "unsupported device type"};
            }
            if (record.header.configVersion > descriptor->currentConfigVersion) {
                snapshot = {};
                return {DeviceError::InvalidVersion, "unsupported device config version"};
            }
        }

        if (recordsById.find(record.header.deviceId) != recordsById.end()) {
            snapshot = {};
            return {DeviceError::DuplicateDeviceId, "duplicate device record"};
        }

        recordsById.emplace(record.header.deviceId, record);
    }

    snapshot.records.clear();
    snapshot.records.reserve(recordsById.size());
    for (const auto& entry : snapshot.indexEntries) {
        snapshot.records.push_back(recordsById.at(entry.deviceId));
    }

    const DeviceValidationResult structureResult = DeviceRegistrySnapshotValidator::validateStructure(snapshot);
    if (!structureResult.ok()) {
        snapshot = {};
        return structureResult;
    }

    const DeviceValidationResult typedRelationshipResult =
        DeviceRegistrySnapshotValidator::validateTypedRelationships(snapshot, typeRegistry);
    if (!typedRelationshipResult.ok()) {
        snapshot = {};
        return typedRelationshipResult;
    }

    return {};
}

DeviceValidationResult DeviceRegistryStore::save(const DeviceRegistrySnapshot& snapshot) {
    DeviceRegistrySnapshot normalized = snapshot;
    if (normalized.indexEntries.empty() && !normalized.records.empty()) {
        normalized.indexEntries.reserve(normalized.records.size());
        for (const auto& record : normalized.records) {
            normalized.indexEntries.push_back({record.header.deviceId, record.header.typeId});
        }
    }

    DeviceValidationResult structureResult = DeviceRegistrySnapshotValidator::validateStructure(normalized);
    if (!structureResult.ok()) {
        return structureResult;
    }

    const std::string indexBlob = DeviceRegistryBinaryCodec::serializeIndex(normalized);
    if (indexBlob.size() > kMaxRegistryIndexBytes) {
        return {DeviceError::BoundsExceeded, "registry index exceeds supported size"};
    }

    if (!storage_.putString(kIndexKey, DeviceRegistryBinaryCodec::toHex(indexBlob))) {
        return {DeviceError::StorageError, "failed to persist registry index"};
    }

    for (const auto& record : normalized.records) {
        const std::string recordBlob = DeviceRegistryBinaryCodec::serializeRecord(record);
        if (recordBlob.size() > kMaxDeviceRecordBytes) {
            return {DeviceError::BoundsExceeded, "device record exceeds supported size"};
        }
        const std::string recordKey = DeviceRegistryBinaryCodec::makeRecordKey(record.header.deviceId);
        if (!storage_.putString(recordKey.c_str(), DeviceRegistryBinaryCodec::toHex(recordBlob))) {
            return {DeviceError::StorageError, "failed to persist device record"};
        }
    }

    return {};
}

} // namespace ewfm
