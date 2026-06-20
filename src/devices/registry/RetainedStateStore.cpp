#include "devices/registry/RetainedStateStore.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device_retained";

struct RetainedStateStorage {
    uint16_t recordVersion{kRetainedStateRecordVersion};
    uint16_t reserved{0};
    DeviceId deviceId{0};
    uint32_t payloadLength{0};
    uint32_t payloadChecksum{0};
    std::array<uint8_t, kMaxRetainedStateBytes> payload{};
};

static_assert(std::is_trivially_copyable<RetainedStateStorage>::value, "retained state storage must be trivially copyable");
static_assert(sizeof(RetainedStateStorage) <= kMaxRetainedStateBytes + 32, "retained state storage unexpectedly large");

bool makeStateKey(char* buffer, size_t bufferSize, DeviceId deviceId) {
    return std::snprintf(buffer, bufferSize, "state_%08x", static_cast<unsigned>(deviceId)) >= 0;
}

uint32_t fnv1a32(const uint8_t* data, size_t size) {
    uint32_t hash = 2166136261UL;
    for (size_t index = 0; index < size; ++index) {
        hash ^= data[index];
        hash *= 16777619UL;
    }
    return hash;
}
} // namespace

RetainedStateStore::RetainedStateStore(IConfigStorage& storage) : storage_(storage) {}

bool RetainedStateStore::begin(bool readOnly) {
    return storage_.begin(kNamespace, readOnly);
}

DeviceValidationResult RetainedStateStore::load(DeviceId deviceId, RetainedStateRecord& record) {
    record = {};
    record.deviceId = deviceId;

    char key[32];
    if (!makeStateKey(key, sizeof(key), deviceId) || !storage_.hasKey(key)) {
        return {DeviceError::MissingRecord, "retained state is missing"};
    }

    RetainedStateStorage storage{};
    if (!getStruct(storage_, key, storage)) {
        return {DeviceError::CorruptRecord, "retained state storage is invalid"};
    }

    if (storage.recordVersion != kRetainedStateRecordVersion) {
        return {DeviceError::InvalidVersion, "unsupported retained state version"};
    }
    if (storage.deviceId == 0) {
        return {DeviceError::InvalidDeviceId, "retained state device id is invalid"};
    }
    if (storage.payloadLength > kMaxRetainedStateBytes) {
        return {DeviceError::BoundsExceeded, "retained state exceeds supported size"};
    }
    if (storage.payloadChecksum != fnv1a32(storage.payload.data(), storage.payloadLength)) {
        return {DeviceError::InvalidConfig, "retained state checksum mismatch"};
    }

    record.recordVersion = storage.recordVersion;
    record.deviceId = storage.deviceId;
    record.payload.assign(reinterpret_cast<const char*>(storage.payload.data()), storage.payloadLength);
    return {};
}

DeviceValidationResult RetainedStateStore::save(const RetainedStateRecord& record) {
    if (record.deviceId == 0) {
        return {DeviceError::InvalidDeviceId, "retained state device id is invalid"};
    }
    if (record.payload.size() > kMaxRetainedStateBytes) {
        return {DeviceError::BoundsExceeded, "retained state exceeds supported size"};
    }

    RetainedStateStorage storage{};
    storage.recordVersion = record.recordVersion == 0 ? kRetainedStateRecordVersion : record.recordVersion;
    storage.deviceId = record.deviceId;
    storage.payloadLength = static_cast<uint32_t>(record.payload.size());
    storage.payloadChecksum = fnv1a32(reinterpret_cast<const uint8_t*>(record.payload.data()), record.payload.size());
    if (!record.payload.empty()) {
        std::memcpy(storage.payload.data(), record.payload.data(), record.payload.size());
    }

    char key[32];
    if (!makeStateKey(key, sizeof(key), record.deviceId) || !putStruct(storage_, key, storage)) {
        return {DeviceError::StorageError, "failed to persist retained state"};
    }

    return {};
}

bool RetainedStateStore::remove(DeviceId deviceId) {
    char key[32];
    return makeStateKey(key, sizeof(key), deviceId) && storage_.remove(key);
}

} // namespace ewfm
