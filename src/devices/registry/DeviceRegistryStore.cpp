#include "devices/registry/DeviceRegistryStore.h"

#include "devices/core/DeviceBaseConfig.h"
#include "devices/registry/DeviceRegistryBinaryCodec.h"
#include "devices/registry/DeviceRegistrySnapshotValidator.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device_registry";
constexpr const char* kRegistryVersionKey = "version";
constexpr const char* kIndexKey = "index";
constexpr uint32_t kRegistryFormatVersion = 3;

struct DeviceRegistryIndexStorageEntry {
    DeviceId deviceId{0};
    DeviceTypeId typeId{0};
    uint16_t reserved{0};
};

struct DeviceRegistryIndexStorage {
    uint16_t recordVersion{kDeviceRegistryIndexVersion};
    uint16_t entryCount{0};
    std::array<DeviceRegistryIndexStorageEntry, kMaxDynamicDevices> entries{};
};

static_assert(std::is_trivially_copyable<DeviceRegistryIndexStorage>::value, "registry index storage must be trivially copyable");
static_assert(sizeof(DeviceRegistryIndexStorage) <= kMaxRegistryIndexBytes, "registry index storage exceeds supported size");

bool recordKey(char* buffer, size_t bufferSize, DeviceId deviceId) {
    return std::snprintf(buffer, bufferSize, "record_%08x", static_cast<unsigned>(deviceId)) >= 0;
}

void resetRegistryStorage(IConfigStorage& storage) {
    (void)storage.clear();
}

std::vector<DeviceIndexEntry> normalizedIndexEntries(const DeviceRegistrySnapshot& snapshot) {
    if (!snapshot.indexEntries.empty()) {
        return snapshot.indexEntries;
    }

    std::vector<DeviceIndexEntry> entries;
    entries.reserve(snapshot.records.size());
    for (const auto& record : snapshot.records) {
        entries.push_back({record.header.deviceId, record.header.typeId});
    }
    return entries;
}

DeviceValidationResult readIndex(IConfigStorage& storage, DeviceRegistrySnapshot& snapshot) {
    if (!storage.hasKey(kIndexKey)) {
        return {};
    }

    DeviceRegistryIndexStorage indexStorage{};
    if (!getStruct(storage, kIndexKey, indexStorage)) {
        return {DeviceError::CorruptRecord, "registry index is invalid"};
    }
    if (indexStorage.recordVersion != kDeviceRegistryIndexVersion) {
        return {DeviceError::InvalidVersion, "unsupported registry index version"};
    }
    if (indexStorage.entryCount > kMaxDynamicDevices) {
        return {DeviceError::BoundsExceeded, "registry index exceeds supported device count"};
    }

    snapshot.indexEntries.clear();
    snapshot.indexEntries.reserve(indexStorage.entryCount);
    for (uint16_t index = 0; index < indexStorage.entryCount; ++index) {
        const auto& entry = indexStorage.entries[index];
        snapshot.indexEntries.push_back({entry.deviceId, entry.typeId});
    }
    return {};
}

DeviceValidationResult readRecord(IConfigStorage& storage, DeviceId deviceId, DeviceTypeId typeId, DeviceRegistryEntry& record,
                                  DeviceConfigBlob& configBlob) {
    char key[32];
    if (!recordKey(key, sizeof(key), deviceId)) {
        return {DeviceError::StorageError, "failed to build device record key"};
    }

    uint8_t buffer[kMaxDeviceRecordBytes]{};
    size_t recordSize = sizeof(buffer);
    if (!storage.getBlob(key, buffer, recordSize)) {
        return {DeviceError::MissingRecord, "missing device record"};
    }

    const DeviceValidationResult parseResult = DeviceRegistryBinaryCodec::parseRecord(buffer, recordSize, record, configBlob);
    if (!parseResult.ok()) {
        return parseResult;
    }
    if (record.header.deviceId != deviceId || record.header.typeId != typeId) {
        return {DeviceError::InvalidConfig, "index entry does not match device record"};
    }
    return {};
}

DeviceValidationResult saveRecord(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob, IConfigStorage& storage) {
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "device record exceeds supported size"};
    }

    char key[32];
    uint8_t buffer[kMaxDeviceRecordBytes]{};
    const size_t recordSize = DeviceRegistryBinaryCodec::serializeRecord(record, configBlob, buffer, sizeof(buffer));
    if (recordSize == 0U || !recordKey(key, sizeof(key), record.header.deviceId) || !storage.putBlob(key, buffer, recordSize)) {
        return {DeviceError::StorageError, "failed to persist device record"};
    }
    return {};
}

DeviceValidationResult saveIndexStorage(const DeviceRegistrySnapshot& snapshot, IConfigStorage& storage) {
    const std::vector<DeviceIndexEntry> entries = normalizedIndexEntries(snapshot);
    if (entries.size() > kMaxDynamicDevices) {
        return {DeviceError::BoundsExceeded, "registry index exceeds supported size"};
    }

    DeviceRegistryIndexStorage indexStorage{};
    indexStorage.entryCount = static_cast<uint16_t>(entries.size());
    for (size_t index = 0; index < entries.size(); ++index) {
        indexStorage.entries[index].deviceId = entries[index].deviceId;
        indexStorage.entries[index].typeId = entries[index].typeId;
    }

    if (!putStruct(storage, kIndexKey, indexStorage)) {
        return {DeviceError::StorageError, "failed to persist registry index"};
    }
    return {};
}
} // namespace

DeviceRegistryStore::DeviceRegistryStore(IConfigStorage& storage) : storage_(storage) {}

bool DeviceRegistryStore::begin(bool readOnly) {
    return storage_.begin(kNamespace, readOnly);
}

DeviceValidationResult DeviceRegistryStore::load(DeviceRegistrySnapshot& snapshot, DeviceConfigBlobMap& configBlobs,
                                                 const DeviceTypeRegistry* typeRegistry) {
    snapshot = {};
    configBlobs.clear();

    uint32_t registryVersion{0};
    if (!storage_.getUInt(kRegistryVersionKey, registryVersion) || registryVersion != kRegistryFormatVersion) {
        resetRegistryStorage(storage_);
        return {};
    }

    const DeviceValidationResult indexResult = readIndex(storage_, snapshot);
    if (!indexResult.ok()) {
        resetRegistryStorage(storage_);
        return {};
    }
    if (snapshot.indexEntries.empty()) {
        return {};
    }

    snapshot.records.clear();
    snapshot.records.reserve(snapshot.indexEntries.size());
    for (const auto& entry : snapshot.indexEntries) {
        DeviceRegistryEntry record{};
        DeviceConfigBlob configBlob{};
        const DeviceValidationResult recordResult = readRecord(storage_, entry.deviceId, entry.typeId, record, configBlob);
        if (!recordResult.ok()) {
            snapshot = {};
            configBlobs.clear();
            resetRegistryStorage(storage_);
            return {};
        }

        if (typeRegistry != nullptr) {
            const DeviceTypeDescriptor* descriptor = typeRegistry->find(record.header.typeId);
            if (descriptor == nullptr) {
                snapshot = {};
                configBlobs.clear();
                resetRegistryStorage(storage_);
                return {};
            }
            if (record.header.configVersion > descriptor->currentConfigVersion) {
                snapshot = {};
                configBlobs.clear();
                resetRegistryStorage(storage_);
                return {};
            }
        }
        configBlobs[record.header.deviceId] = configBlob;
        snapshot.records.push_back(std::move(record));
    }

    const DeviceValidationResult structureResult = DeviceRegistrySnapshotValidator::validateStructure(snapshot);
    if (!structureResult.ok()) {
        snapshot = {};
        configBlobs.clear();
        resetRegistryStorage(storage_);
        return {};
    }

    const DeviceValidationResult typedRelationshipResult =
        DeviceRegistrySnapshotValidator::validateTypedRelationships(snapshot, typeRegistry);
    if (!typedRelationshipResult.ok()) {
        snapshot = {};
        configBlobs.clear();
        resetRegistryStorage(storage_);
        return {};
    }

    return {};
}

DeviceValidationResult DeviceRegistryStore::save(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs) {
    DeviceRegistrySnapshot normalized = snapshot;
    normalized.indexEntries = normalizedIndexEntries(normalized);
    const DeviceValidationResult structureResult = DeviceRegistrySnapshotValidator::validateStructure(normalized);
    if (!structureResult.ok()) {
        return structureResult;
    }

    const DeviceValidationResult recordResult = saveRecords(normalized, configBlobs, {});
    if (!recordResult.ok()) {
        return recordResult;
    }

    const DeviceValidationResult indexResult = saveIndex(normalized);
    return indexResult;
}

DeviceValidationResult DeviceRegistryStore::saveRecords(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs,
                                                        const std::vector<DeviceId>& dirtyRecordIds) {
    const bool saveAllRecords = dirtyRecordIds.empty();
    for (const auto& record : snapshot.records) {
        if (!saveAllRecords && std::find(dirtyRecordIds.begin(), dirtyRecordIds.end(), record.header.deviceId) == dirtyRecordIds.end()) {
            continue;
        }

        const auto configIt = configBlobs.find(record.header.deviceId);
        if (configIt == configBlobs.end()) {
            return {DeviceError::MissingRecord, "missing device config"};
        }
        const DeviceValidationResult recordResult = saveRecord(record, configIt->second, storage_);
        if (!recordResult.ok()) {
            return recordResult;
        }
    }
    return {};
}

DeviceValidationResult DeviceRegistryStore::saveIndex(const DeviceRegistrySnapshot& snapshot) {
    const DeviceValidationResult indexResult = saveIndexStorage(snapshot, storage_);
    if (!indexResult.ok()) {
        return indexResult;
    }

    if (!storage_.putUInt(kRegistryVersionKey, kRegistryFormatVersion)) {
        return {DeviceError::StorageError, "failed to persist registry version"};
    }

    return {};
}

bool DeviceRegistryStore::removeRecord(DeviceId deviceId) {
    char key[32];
    return recordKey(key, sizeof(key), deviceId) && storage_.remove(key);
}

} // namespace ewfm
