#pragma once

#include "config/ConfigStore.h"
#include "devices/core/DeviceTypes.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>

namespace ewfm {

class DeviceScopedDataStore {
public:
    explicit DeviceScopedDataStore(IConfigStorage& storage);

    bool begin(bool readOnly = false);

    template <typename TRecord> DeviceValidationResult load(DeviceId deviceId, const char* dataType, TRecord& record) {
        static_assert(std::is_trivially_copyable<TRecord>::value, "device scoped record must be trivially copyable");
        static_assert(sizeof(TRecord) <= kMaxRetainedStateBytes, "device scoped record exceeds storage limit");

        record = {};
        record.deviceId = deviceId;

        bool opened = false;
        if (!openDeviceNamespace(deviceId, true, opened)) {
            return {DeviceError::MissingRecord, "device scoped data is missing"};
        }

        const std::string key = makeDataKey(dataType);
        if (!storage_.hasKey(key.c_str())) {
            if (opened) {
                storage_.end();
            }
            return {DeviceError::MissingRecord, "device scoped data is missing"};
        }

        TRecord storage{};
        if (!getStruct(storage_, key.c_str(), storage)) {
            (void)storage_.remove(key.c_str());
            if (opened) {
                storage_.end();
            }
            return {DeviceError::MissingRecord, "device scoped data is missing"};
        }

        if (storage.recordVersion == 0 || storage.deviceId == 0) {
            (void)storage_.remove(key.c_str());
            if (opened) {
                storage_.end();
            }
            return {DeviceError::MissingRecord, "device scoped data is missing"};
        }

        record = storage;
        if (opened) {
            storage_.end();
        }
        return {};
    }

    template <typename TRecord> DeviceValidationResult save(DeviceId deviceId, const char* dataType, const TRecord& record) {
        static_assert(std::is_trivially_copyable<TRecord>::value, "device scoped record must be trivially copyable");
        static_assert(sizeof(TRecord) <= kMaxRetainedStateBytes, "device scoped record exceeds storage limit");

        if (record.deviceId == 0 || record.deviceId != deviceId) {
            return {DeviceError::InvalidDeviceId, "device scoped data device id is invalid"};
        }

        bool opened = false;
        if (!openDeviceNamespace(deviceId, false, opened)) {
            return {DeviceError::StorageError, "failed to open device scoped storage"};
        }

        TRecord storage = record;
        storage.recordVersion = storage.recordVersion == 0 ? kRetainedStateRecordVersion : storage.recordVersion;
        const std::string key = makeDataKey(dataType);
        const bool ok = putStruct(storage_, key.c_str(), storage);
        if (opened) {
            storage_.end();
        }
        if (!ok) {
            return {DeviceError::StorageError, "failed to persist device scoped data"};
        }
        return {};
    }

    bool remove(DeviceId deviceId, const char* dataType);
    bool clearDevice(DeviceId deviceId);

private:
    static bool makeNamespace(char* buffer, size_t bufferSize, DeviceId deviceId);
    static std::string makeDataKey(const char* dataType);
    bool openDeviceNamespace(DeviceId deviceId, bool readOnly, bool& opened);

    IConfigStorage& storage_;
};

} // namespace ewfm
