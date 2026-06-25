#pragma once

#include "devices/registry/DeviceScopedDataStore.h"

#include <cstdio>
#include <type_traits>

namespace ewfm {

class DeviceRetainedDataStore {
public:
    explicit DeviceRetainedDataStore(IConfigStorage& storage);

    bool begin(bool readOnly = false);

    template <typename TRecord> DeviceValidationResult load(DeviceId deviceId, TRecord& record) {
        static_assert(std::is_trivially_copyable<TRecord>::value, "retained state record must be trivially copyable");
        static_assert(sizeof(TRecord) <= kMaxRetainedStateBytes, "retained state record exceeds storage limit");

        record = {};
        record.deviceId = deviceId;

        TRecord storage{};
        const DeviceValidationResult result = storage_.load(deviceId, kRetainedStateDataType, storage);
        if (!result.ok()) {
            if (result.error != DeviceError::MissingRecord) {
                return result;
            }
            return loadLegacy(deviceId, record);
        }

        if (storage.recordVersion != kRetainedStateRecordVersion || storage.deviceId == 0) {
            (void)storage_.remove(deviceId, kRetainedStateDataType);
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
        return storage_.save(record.deviceId, kRetainedStateDataType, storage);
    }

    bool remove(DeviceId deviceId);
    bool clearDevice(DeviceId deviceId);
    bool clearLegacyNamespace();

private:
    static constexpr const char* kLegacyNamespace = "device_retained";
    static constexpr const char* kRetainedStateDataType = "retained_state";
    static constexpr const char* kLegacyRetainedStateKeyPrefix = "state_";

    template <typename TRecord> DeviceValidationResult loadLegacy(DeviceId deviceId, TRecord& record);
    static bool makeLegacyKey(char* buffer, size_t bufferSize, DeviceId deviceId);

    IConfigStorage& legacyStorage_;
    DeviceScopedDataStore storage_;
};

template <typename TRecord> DeviceValidationResult DeviceRetainedDataStore::loadLegacy(DeviceId deviceId, TRecord& record) {
    static_assert(std::is_trivially_copyable<TRecord>::value, "retained state record must be trivially copyable");
    static_assert(sizeof(TRecord) <= kMaxRetainedStateBytes, "retained state record exceeds storage limit");

    record = {};
    record.deviceId = deviceId;

    if (!legacyStorage_.begin(kLegacyNamespace, false)) {
        return {DeviceError::StorageError, "failed to open legacy retained state storage"};
    }

    char key[32];
    if (!makeLegacyKey(key, sizeof(key), deviceId) || !legacyStorage_.hasKey(key)) {
        legacyStorage_.end();
        return {DeviceError::MissingRecord, "retained state is missing"};
    }

    TRecord legacy{};
    if (!getStruct(legacyStorage_, key, legacy)) {
        (void)legacyStorage_.remove(key);
        legacyStorage_.end();
        return {DeviceError::MissingRecord, "retained state is missing"};
    }

    legacyStorage_.end();
    if (legacy.recordVersion != kRetainedStateRecordVersion || legacy.deviceId == 0) {
        if (legacyStorage_.begin(kLegacyNamespace, false)) {
            (void)legacyStorage_.remove(key);
            legacyStorage_.end();
        }
        return {DeviceError::MissingRecord, "retained state is missing"};
    }

    record = legacy;
    const DeviceValidationResult saveResult = storage_.save(deviceId, kRetainedStateDataType, record);
    if (!saveResult.ok()) {
        return saveResult;
    }
    if (legacyStorage_.begin(kLegacyNamespace, false)) {
        (void)legacyStorage_.remove(key);
        legacyStorage_.end();
    }
    return {};
}

} // namespace ewfm
