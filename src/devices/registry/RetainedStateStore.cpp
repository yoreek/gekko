#include "devices/registry/RetainedStateStore.h"

#include <cstdio>
#include <type_traits>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device_retained";
static_assert(std::is_trivially_copyable<RetainedStateRecord>::value, "retained state record must be trivially copyable");

bool makeStateKey(char* buffer, size_t bufferSize, DeviceId deviceId) {
    return std::snprintf(buffer, bufferSize, "state_%08x", static_cast<unsigned>(deviceId)) >= 0;
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

    RetainedStateRecord storage{};
    if (!getStruct(storage_, key, storage)) {
        (void)storage_.remove(key);
        return {DeviceError::MissingRecord, "retained state is missing"};
    }

    if (storage.recordVersion != kRetainedStateRecordVersion) {
        (void)storage_.remove(key);
        return {DeviceError::MissingRecord, "retained state is missing"};
    }
    if (storage.deviceId == 0) {
        (void)storage_.remove(key);
        return {DeviceError::MissingRecord, "retained state is missing"};
    }

    record.recordVersion = storage.recordVersion;
    record.deviceId = storage.deviceId;
    record.outputState = storage.outputState;
    return {};
}

DeviceValidationResult RetainedStateStore::save(const RetainedStateRecord& record) {
    if (record.deviceId == 0) {
        return {DeviceError::InvalidDeviceId, "retained state device id is invalid"};
    }
    RetainedStateRecord storage = record;
    storage.recordVersion = record.recordVersion == 0 ? kRetainedStateRecordVersion : record.recordVersion;

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
