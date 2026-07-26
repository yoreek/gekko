#include "devices/registry/DeviceRegistryStore.h"

#include "debug/Debug.h"
#include "devices/core/DeviceBaseConfig.h"
#include "devices/registry/DeviceRegistryBinaryCodec.h"
#include "devices/registry/DeviceRegistrySnapshotValidator.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <type_traits>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device_registry";
constexpr const char* kRegistryVersionKey = "version";
constexpr const char* kIndexKey = "index";
constexpr uint32_t kRegistryFormatVersion = 4;

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
                                                 const DeviceTypeRegistry* typeRegistry, std::vector<DeviceId>* discardedDeviceIds) {
    snapshot = {};
    configBlobs.clear();
    if (discardedDeviceIds != nullptr) {
        discardedDeviceIds->clear();
    }

    uint32_t registryVersion{0};
    if (!storage_.getUInt(kRegistryVersionKey, registryVersion)) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("load: registry version key missing, resetting storage");
        resetRegistryStorage(storage_);
        return {};
    }
    if (registryVersion != kRegistryFormatVersion) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("load: registry version mismatch (stored=%u expected=%u), resetting storage",
                                      static_cast<unsigned>(registryVersion), static_cast<unsigned>(kRegistryFormatVersion));
        resetRegistryStorage(storage_);
        return {};
    }

    const DeviceValidationResult indexResult = readIndex(storage_, snapshot);
    if (!indexResult.ok()) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("load: index read failed (%s), resetting storage",
                                      indexResult.message != nullptr ? indexResult.message : "unknown");
        resetRegistryStorage(storage_);
        return {};
    }
    if (snapshot.indexEntries.empty()) {
        EWFM_DEVICE_REGISTRY_LOG_WARN("load: version ok but index is empty/absent (hasIndexKey=%d)", storage_.hasKey(kIndexKey) ? 1 : 0);
        return {};
    }

    std::map<DeviceId, bool> indexIds{};
    for (const DeviceIndexEntry& entry : snapshot.indexEntries) {
        if (entry.deviceId == 0U || entry.typeId == 0U || indexIds.find(entry.deviceId) != indexIds.end()) {
            EWFM_DEVICE_REGISTRY_LOG_WARN("load: registry index contains an invalid entry, resetting storage");
            snapshot = {};
            resetRegistryStorage(storage_);
            return {};
        }
        indexIds.emplace(entry.deviceId, true);
    }

    const std::vector<DeviceIndexEntry> storedEntries = snapshot.indexEntries;
    snapshot.indexEntries.clear();
    snapshot.records.clear();
    snapshot.indexEntries.reserve(storedEntries.size());
    snapshot.records.reserve(storedEntries.size());
    std::vector<DeviceId> discarded{};
    for (const auto& entry : storedEntries) {
        DeviceRegistryEntry record{};
        DeviceConfigBlob configBlob{};
        const DeviceValidationResult recordResult = readRecord(storage_, entry.deviceId, entry.typeId, record, configBlob);
        if (!recordResult.ok()) {
            EWFM_DEVICE_REGISTRY_LOG_WARN("load: discarding unreadable record device=%u type=%u (%s)",
                                          static_cast<unsigned>(entry.deviceId), static_cast<unsigned>(entry.typeId), recordResult.message);
            discarded.push_back(entry.deviceId);
            continue;
        }

        if (typeRegistry != nullptr) {
            const DeviceTypeDescriptor* descriptor = typeRegistry->find(record.header.typeId);
            if (descriptor == nullptr) {
                EWFM_DEVICE_REGISTRY_LOG_WARN("load: discarding unknown device type=%u for device=%u",
                                              static_cast<unsigned>(record.header.typeId), static_cast<unsigned>(record.header.deviceId));
                discarded.push_back(entry.deviceId);
                continue;
            }
            if (record.header.configVersion > descriptor->currentConfigVersion) {
                EWFM_DEVICE_REGISTRY_LOG_WARN("load: discarding too-new config for device=%u type=%u (stored=%u supported=%u)",
                                              static_cast<unsigned>(record.header.deviceId), static_cast<unsigned>(record.header.typeId),
                                              static_cast<unsigned>(record.header.configVersion),
                                              static_cast<unsigned>(descriptor->currentConfigVersion));
                discarded.push_back(entry.deviceId);
                continue;
            }
        }
        configBlobs[record.header.deviceId] = configBlob;
        snapshot.indexEntries.push_back(entry);
        snapshot.records.push_back(std::move(record));
    }

    if (!discarded.empty()) {
        const DeviceValidationResult indexSaveResult = saveIndex(snapshot);
        if (!indexSaveResult.ok()) {
            EWFM_DEVICE_REGISTRY_LOG_WARN("load: unable to persist cleaned index (%s)", indexSaveResult.message);
        } else {
            for (const DeviceId deviceId : discarded) {
                (void)removeRecord(deviceId);
            }
        }
        if (discardedDeviceIds != nullptr) {
            *discardedDeviceIds = discarded;
        }
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

DeviceValidationResult DeviceRegistryStore::saveRecovered(const DeviceRegistrySnapshot& snapshot, const DeviceConfigBlobMap& configBlobs) {
    const DeviceValidationResult recordResult = saveRecords(snapshot, configBlobs, {});
    if (!recordResult.ok()) {
        return recordResult;
    }
    return saveIndex(snapshot);
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
