#include "devices/registry/DeviceRegistryStore.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <map>
#include <sstream>
#include <type_traits>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device_registry";
constexpr const char* kIndexKey = "index";
constexpr uint32_t kRegistryMagic = 0x44565249UL;
constexpr uint32_t kRecordMagic = 0x44565243UL;
constexpr char kHexDigits[] = "0123456789abcdef";

template <typename T> void appendLE(std::string& out, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    const Unsigned v = static_cast<Unsigned>(value);
    for (size_t index = 0; index < sizeof(T); ++index) {
        out.push_back(static_cast<char>((v >> (index * 8)) & 0xFFU));
    }
}

template <typename T> bool readLE(const std::string& blob, size_t& pos, T& value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    if (pos + sizeof(T) > blob.size()) {
        return false;
    }
    Unsigned v{0};
    for (size_t index = 0; index < sizeof(T); ++index) {
        v |= static_cast<Unsigned>(static_cast<unsigned char>(blob[pos + index])) << (index * 8);
    }
    value = static_cast<T>(v);
    pos += sizeof(T);
    return true;
}

void appendBytes(std::string& out, const std::string& bytes) {
    out.append(bytes.data(), bytes.size());
}

std::string toHex(const std::string& bytes) {
    std::string hex;
    hex.reserve(bytes.size() * 2);
    for (unsigned char byte : bytes) {
        hex.push_back(kHexDigits[(byte >> 4) & 0x0F]);
        hex.push_back(kHexDigits[byte & 0x0F]);
    }
    return hex;
}

int fromHexDigit(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    return -1;
}

bool fromHex(const std::string& hex, std::string& bytes) {
    if ((hex.size() % 2) != 0) {
        return false;
    }
    bytes.clear();
    bytes.reserve(hex.size() / 2);
    for (size_t index = 0; index < hex.size(); index += 2) {
        const int hi = fromHexDigit(hex[index]);
        const int lo = fromHexDigit(hex[index + 1]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        bytes.push_back(static_cast<char>((hi << 4) | lo));
    }
    return true;
}

bool readBytes(const std::string& blob, size_t& pos, size_t length, std::string& out) {
    if (pos + length > blob.size()) {
        return false;
    }
    out.assign(blob.data() + pos, length);
    pos += length;
    return true;
}

uint32_t fnv1a32(const std::string& bytes) {
    uint32_t hash = 2166136261UL;
    for (unsigned char byte : bytes) {
        hash ^= byte;
        hash *= 16777619UL;
    }
    return hash;
}

std::string makeRecordKey(DeviceId deviceId) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "record_%08x", static_cast<unsigned>(deviceId));
    return buffer;
}

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

DeviceValidationResult validateSnapshotStructure(const DeviceRegistrySnapshot& snapshot) {
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
        if (record.header.payloadChecksum != 0 && record.header.payloadChecksum != fnv1a32(record.configPayload)) {
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
        const auto parentIt = recordById.find(entry.deviceId);
        if (parentIt == recordById.end()) {
            continue;
        }
        const DeviceRecord& child = *parentIt->second;
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

std::string serializeIndex(const DeviceRegistrySnapshot& snapshot) {
    std::string blob;
    blob.reserve(16 + snapshot.indexEntries.size() * 8);
    appendLE<uint32_t>(blob, kRegistryMagic);
    appendLE<uint16_t>(blob, kDeviceRegistryIndexVersion);
    appendLE<uint16_t>(blob, static_cast<uint16_t>(snapshot.indexEntries.size()));
    for (const auto& entry : snapshot.indexEntries) {
        appendLE<uint32_t>(blob, entry.deviceId);
        appendLE<uint16_t>(blob, entry.typeId);
        appendLE<uint16_t>(blob, 0);
    }
    return blob;
}

DeviceValidationResult parseIndex(const std::string& blob, DeviceRegistrySnapshot& snapshot) {
    size_t pos = 0;
    uint32_t magic{0};
    uint16_t version{0};
    uint16_t count{0};
    if (!readLE(blob, pos, magic) || !readLE(blob, pos, version) || !readLE(blob, pos, count)) {
        return {DeviceError::CorruptRecord, "registry index is truncated"};
    }
    if (magic != kRegistryMagic || version != kDeviceRegistryIndexVersion) {
        return {DeviceError::InvalidVersion, "unsupported registry index version"};
    }
    if (count > kMaxDynamicDevices) {
        return {DeviceError::BoundsExceeded, "registry index exceeds supported device count"};
    }

    snapshot.indexEntries.clear();
    snapshot.indexEntries.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        DeviceIndexEntry entry{};
        uint16_t reserved{0};
        if (!readLE(blob, pos, entry.deviceId) || !readLE(blob, pos, entry.typeId) || !readLE(blob, pos, reserved)) {
            return {DeviceError::CorruptRecord, "registry index entry is truncated"};
        }
        snapshot.indexEntries.push_back(entry);
    }

    if (pos != blob.size()) {
        return {DeviceError::CorruptRecord, "registry index has trailing data"};
    }
    return {};
}

std::string serializeRecord(const DeviceRecord& record) {
    const uint32_t payloadLength =
        record.configPayload.size() > kMaxDeviceConfigBytes ? 0U : static_cast<uint32_t>(record.configPayload.size());
    std::string blob;
    blob.reserve(48 + record.name.size() + record.configPayload.size());
    appendLE<uint32_t>(blob, kRecordMagic);
    appendLE<uint16_t>(blob, kDeviceRecordHeaderVersion);
    appendLE<uint16_t>(blob, 0);
    appendLE<DeviceId>(blob, record.header.deviceId);
    appendLE<DeviceTypeId>(blob, record.header.typeId);
    appendLE<uint16_t>(blob, record.enabled ? 1U : 0U);
    appendLE<uint32_t>(blob, record.header.configVersion);
    appendLE<uint32_t>(blob, record.header.configRevision);
    appendLE<uint32_t>(blob, payloadLength);
    appendLE<uint32_t>(blob, fnv1a32(record.configPayload));
    appendLE<uint32_t>(blob, static_cast<uint32_t>(record.name.size()));
    appendLE<uint8_t>(blob, record.hasParent ? 1U : 0U);
    appendLE<uint8_t>(blob, static_cast<uint8_t>(record.status));
    appendLE<uint8_t>(blob, static_cast<uint8_t>(record.persistencePolicy));
    appendLE<uint8_t>(blob, 0U);
    appendLE<DeviceId>(blob, record.parentDeviceId);
    appendBytes(blob, record.name);
    appendBytes(blob, record.configPayload);
    return blob;
}

DeviceValidationResult parseRecord(const std::string& blob, DeviceRecord& record) {
    size_t pos = 0;
    uint32_t magic{0};
    uint16_t version{0};
    uint16_t reserved16{0};
    uint16_t enabled{0};
    uint32_t nameLength{0};
    uint8_t hasParent{0};
    uint8_t status{0};
    uint8_t persistence{0};
    uint8_t reserved8{0};
    if (!readLE(blob, pos, magic) || !readLE(blob, pos, version) || !readLE(blob, pos, reserved16)) {
        return {DeviceError::CorruptRecord, "device record is truncated"};
    }
    if (magic != kRecordMagic || version != kDeviceRecordHeaderVersion) {
        return {DeviceError::InvalidVersion, "unsupported device record version"};
    }

    if (!readLE(blob, pos, record.header.deviceId) || !readLE(blob, pos, record.header.typeId) || !readLE(blob, pos, enabled) ||
        !readLE(blob, pos, record.header.configVersion) || !readLE(blob, pos, record.header.configRevision) ||
        !readLE(blob, pos, record.header.payloadLength) || !readLE(blob, pos, record.header.payloadChecksum) ||
        !readLE(blob, pos, nameLength) || !readLE(blob, pos, hasParent) || !readLE(blob, pos, status) || !readLE(blob, pos, persistence) ||
        !readLE(blob, pos, reserved8) || !readLE(blob, pos, record.parentDeviceId)) {
        return {DeviceError::CorruptRecord, "device record header is truncated"};
    }

    if (nameLength > kMaxDynamicDeviceNameLength || record.header.payloadLength > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "device record exceeds supported size"};
    }

    if (!readBytes(blob, pos, nameLength, record.name) || !readBytes(blob, pos, record.header.payloadLength, record.configPayload)) {
        return {DeviceError::CorruptRecord, "device record payload is truncated"};
    }

    if (pos != blob.size()) {
        return {DeviceError::CorruptRecord, "device record has trailing data"};
    }

    if (record.header.payloadChecksum != fnv1a32(record.configPayload)) {
        return {DeviceError::InvalidConfig, "device record payload checksum mismatch"};
    }

    record.header.recordVersion = version;
    record.enabled = enabled != 0;
    record.hasParent = hasParent != 0;
    record.status = static_cast<DeviceStatus>(status);
    record.persistencePolicy = static_cast<DevicePersistencePolicy>(persistence);
    return {};
}

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
    if (!fromHex(indexBlob, indexBytes)) {
        return {DeviceError::CorruptRecord, "registry index is not valid hex"};
    }
    indexBlob = std::move(indexBytes);

    DeviceValidationResult indexResult = parseIndex(indexBlob, snapshot);
    if (!indexResult.ok()) {
        snapshot = {};
        return indexResult;
    }

    std::map<DeviceId, DeviceRecord> recordsById;
    for (const auto& entry : snapshot.indexEntries) {
        const std::string recordKey = makeRecordKey(entry.deviceId);
        std::string recordBlob;
        if (!storage_.getString(recordKey.c_str(), recordBlob)) {
            snapshot = {};
            return {DeviceError::MissingRecord, "missing device record"};
        }
        std::string recordBytes;
        if (!fromHex(recordBlob, recordBytes)) {
            snapshot = {};
            return {DeviceError::CorruptRecord, "device record is not valid hex"};
        }
        recordBlob = std::move(recordBytes);

        DeviceRecord record{};
        DeviceValidationResult recordResult = parseRecord(recordBlob, record);
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

    std::map<DeviceId, size_t> childCounts;
    for (const auto& entry : snapshot.indexEntries) {
        const DeviceRecord& record = recordsById.at(entry.deviceId);
        if (!record.hasParent) {
            continue;
        }

        const auto parentIt = recordsById.find(record.parentDeviceId);
        if (parentIt == recordsById.end()) {
            snapshot = {};
            return {DeviceError::InvalidRelationship, "missing parent record"};
        }

        const DeviceRecord& parentRecord = parentIt->second;
        const DeviceTypeDescriptor* childDescriptor = typeRegistry != nullptr ? typeRegistry->find(record.header.typeId) : nullptr;
        const DeviceTypeDescriptor* parentDescriptor = typeRegistry != nullptr ? typeRegistry->find(parentRecord.header.typeId) : nullptr;

        if (typeRegistry != nullptr && (childDescriptor == nullptr || parentDescriptor == nullptr)) {
            snapshot = {};
            return {DeviceError::UnsupportedType, "unsupported device type"};
        }

        if (typeRegistry != nullptr && childDescriptor != nullptr && !childDescriptor->compatibleParentTypes.empty()) {
            const bool parentCompatible =
                std::find(childDescriptor->compatibleParentTypes.begin(), childDescriptor->compatibleParentTypes.end(),
                          parentRecord.header.typeId) != childDescriptor->compatibleParentTypes.end();
            if (!parentCompatible) {
                snapshot = {};
                return {DeviceError::InvalidRelationship, "incompatible parent type"};
            }
        }

        if (typeRegistry != nullptr && parentDescriptor != nullptr && !parentDescriptor->canHaveChildren) {
            snapshot = {};
            return {DeviceError::InvalidRelationship, "parent type cannot have children"};
        }

        const size_t nextChildCount = ++childCounts[record.parentDeviceId];
        if (typeRegistry != nullptr && parentDescriptor != nullptr && parentDescriptor->maxChildren > 0 &&
            nextChildCount > parentDescriptor->maxChildren) {
            snapshot = {};
            return {DeviceError::InvalidRelationship, "parent child limit exceeded"};
        }
    }

    snapshot.records.clear();
    snapshot.records.reserve(recordsById.size());
    for (const auto& entry : snapshot.indexEntries) {
        snapshot.records.push_back(recordsById.at(entry.deviceId));
    }

    const DeviceValidationResult structureResult = validateSnapshotStructure(snapshot);
    if (!structureResult.ok()) {
        snapshot = {};
        return structureResult;
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

    DeviceValidationResult structureResult = validateSnapshotStructure(normalized);
    if (!structureResult.ok()) {
        return structureResult;
    }

    const std::string indexBlob = serializeIndex(normalized);
    if (indexBlob.size() > kMaxRegistryIndexBytes) {
        return {DeviceError::BoundsExceeded, "registry index exceeds supported size"};
    }

    if (!storage_.putString(kIndexKey, toHex(indexBlob))) {
        return {DeviceError::StorageError, "failed to persist registry index"};
    }

    for (const auto& record : normalized.records) {
        const std::string recordBlob = serializeRecord(record);
        if (recordBlob.size() > kMaxDeviceRecordBytes) {
            return {DeviceError::BoundsExceeded, "device record exceeds supported size"};
        }
        const std::string recordKey = makeRecordKey(record.header.deviceId);
        if (!storage_.putString(recordKey.c_str(), toHex(recordBlob))) {
            return {DeviceError::StorageError, "failed to persist device record"};
        }
    }

    return {};
}

} // namespace ewfm
