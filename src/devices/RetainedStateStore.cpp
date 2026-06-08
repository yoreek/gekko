#include "devices/RetainedStateStore.h"

#include <cstdio>
#include <type_traits>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device_retained";
constexpr uint32_t kRetainedMagic = 0x44525444UL;

template <typename T>
void appendLE(std::string& out, T value) {
    using Unsigned = typename std::make_unsigned<T>::type;
    const Unsigned v = static_cast<Unsigned>(value);
    for (size_t index = 0; index < sizeof(T); ++index) {
        out.push_back(static_cast<char>((v >> (index * 8)) & 0xFFU));
    }
}

template <typename T>
bool readLE(const std::string& blob, size_t& pos, T& value) {
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

std::string makeStateKey(DeviceId deviceId) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "state_%08x", static_cast<unsigned>(deviceId));
    return buffer;
}

std::string serializeRecord(const RetainedStateRecord& record) {
    std::string blob;
    blob.reserve(24 + record.payload.size());
    appendLE<uint32_t>(blob, kRetainedMagic);
    appendLE<uint16_t>(blob, record.recordVersion == 0 ? kRetainedStateRecordVersion : record.recordVersion);
    appendLE<uint16_t>(blob, 0);
    appendLE<DeviceId>(blob, record.deviceId);
    appendLE<uint32_t>(blob, static_cast<uint32_t>(record.payload.size()));
    appendLE<uint32_t>(blob, fnv1a32(record.payload));
    appendBytes(blob, record.payload);
    return blob;
}

DeviceValidationResult parseRecord(const std::string& blob, RetainedStateRecord& record) {
    size_t pos = 0;
    uint32_t magic{0};
    uint16_t version{0};
    uint16_t reserved{0};
    uint32_t payloadLength{0};
    uint32_t checksum{0};
    if (!readLE(blob, pos, magic) || !readLE(blob, pos, version) || !readLE(blob, pos, reserved) || !readLE(blob, pos, record.deviceId) ||
        !readLE(blob, pos, payloadLength) || !readLE(blob, pos, checksum)) {
        return {DeviceError::CorruptRecord, "retained state record is truncated"};
    }

    if (magic != kRetainedMagic || version != kRetainedStateRecordVersion) {
        return {DeviceError::InvalidVersion, "unsupported retained state version"};
    }
    if (record.deviceId == 0) {
        return {DeviceError::InvalidDeviceId, "retained state device id is invalid"};
    }
    if (payloadLength > kMaxRetainedStateBytes) {
        return {DeviceError::BoundsExceeded, "retained state exceeds supported size"};
    }

    if (!readBytes(blob, pos, payloadLength, record.payload)) {
        return {DeviceError::CorruptRecord, "retained state record is truncated"};
    }
    if (pos != blob.size()) {
        return {DeviceError::CorruptRecord, "retained state record has trailing data"};
    }
    if (checksum != fnv1a32(record.payload)) {
        return {DeviceError::InvalidConfig, "retained state checksum mismatch"};
    }

    record.recordVersion = version;
    return {};
}

} // namespace

RetainedStateStore::RetainedStateStore(IConfigStorage& storage) : storage_(storage) {}

bool RetainedStateStore::begin(bool readOnly) {
    return storage_.begin(kNamespace, readOnly);
}

DeviceValidationResult RetainedStateStore::load(DeviceId deviceId, RetainedStateRecord& record) {
    record = {};
    record.deviceId = deviceId;

    const std::string key = makeStateKey(deviceId);
    if (!storage_.hasKey(key.c_str())) {
        return {DeviceError::MissingRecord, "retained state is missing"};
    }

    std::string blob;
    if (!storage_.getString(key.c_str(), blob)) {
        return {DeviceError::StorageError, "failed to read retained state"};
    }

    DeviceValidationResult result = parseRecord(blob, record);
    if (!result.ok()) {
        record = {};
        record.deviceId = deviceId;
        return result;
    }

    return {};
}

DeviceValidationResult RetainedStateStore::save(const RetainedStateRecord& record) {
    if (record.deviceId == 0) {
        return {DeviceError::InvalidDeviceId, "retained state device id is invalid"};
    }
    if (record.payload.size() > kMaxRetainedStateBytes) {
        return {DeviceError::BoundsExceeded, "retained state exceeds supported size"};
    }

    const std::string blob = serializeRecord(record);
    if (!storage_.putString(makeStateKey(record.deviceId).c_str(), blob)) {
        return {DeviceError::StorageError, "failed to persist retained state"};
    }

    return {};
}

bool RetainedStateStore::remove(DeviceId deviceId) {
    return storage_.remove(makeStateKey(deviceId).c_str());
}

} // namespace ewfm
