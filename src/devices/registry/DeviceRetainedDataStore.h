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
            return result;
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

private:
    static constexpr const char* kRetainedStateDataType = "retained_state";
    DeviceScopedDataStore storage_;
};

} // namespace ewfm
