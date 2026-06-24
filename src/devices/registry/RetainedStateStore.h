#pragma once

#include "config/ConfigStore.h"
#include "devices/core/DeviceTypes.h"

#include <cstdio>
#include <type_traits>

namespace ewfm {

class RetainedStateStore {
public:
    explicit RetainedStateStore(IConfigStorage& storage);

    bool begin(bool readOnly = false);

    template <typename TRecord> DeviceValidationResult load(DeviceId deviceId, TRecord& record) {
        static_assert(std::is_trivially_copyable<TRecord>::value, "retained state record must be trivially copyable");
        static_assert(sizeof(TRecord) <= kMaxRetainedStateBytes, "retained state record exceeds storage limit");

        record = {};
        record.deviceId = deviceId;

        char key[32];
        if (!makeStateKey(key, sizeof(key), deviceId) || !storage_.hasKey(key)) {
            return {DeviceError::MissingRecord, "retained state is missing"};
        }

        TRecord storage{};
        if (!getStruct(storage_, key, storage)) {
            (void)storage_.remove(key);
            return {DeviceError::MissingRecord, "retained state is missing"};
        }

        if (storage.recordVersion != kRetainedStateRecordVersion || storage.deviceId == 0) {
            (void)storage_.remove(key);
            return {DeviceError::MissingRecord, "retained state is missing"};
        }

        record = storage;
        return {};
    }

    template <typename TRecord> DeviceValidationResult save(const TRecord& record) {
        static_assert(std::is_trivially_copyable<TRecord>::value, "retained state record must be trivially copyable");
        static_assert(sizeof(TRecord) <= kMaxRetainedStateBytes, "retained state record exceeds storage limit");

        if (record.deviceId == 0) {
            return {DeviceError::InvalidDeviceId, "retained state device id is invalid"};
        }

        TRecord storage = record;
        storage.recordVersion = storage.recordVersion == 0 ? kRetainedStateRecordVersion : storage.recordVersion;

        char key[32];
        if (!makeStateKey(key, sizeof(key), record.deviceId) || !putStruct(storage_, key, storage)) {
            return {DeviceError::StorageError, "failed to persist retained state"};
        }

        return {};
    }

    bool remove(DeviceId deviceId);

private:
    static bool makeStateKey(char* buffer, size_t bufferSize, DeviceId deviceId);

    IConfigStorage& storage_;
};

} // namespace ewfm
