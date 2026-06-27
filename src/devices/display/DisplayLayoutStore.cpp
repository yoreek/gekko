#include "devices/display/DisplayLayoutStore.h"

#include "devices/display/DisplayLayoutCodec.h"

#include <cstring>

namespace ewfm {

bool DisplayLayoutStore::begin(bool readOnly) {
    return storage_ != nullptr && storage_->begin(readOnly);
}

DeviceValidationResult DisplayLayoutStore::load(DeviceId deviceId, DisplayLayoutRecordV1& record) {
    if (storage_ == nullptr) {
        return {DeviceError::StorageError, "display layout store is unavailable"};
    }
    std::vector<uint8_t> blob;
    const DeviceValidationResult result = storage_->loadBlob(deviceId, kDataType, blob);
    if (!result.ok()) {
        return result;
    }
    if (!decodeDisplayLayoutBinary(blob.data(), blob.size(), record)) {
        return {DeviceError::InvalidConfig, "display layout payload is invalid"};
    }
    return {};
}

DeviceValidationResult DisplayLayoutStore::save(const DisplayLayoutRecordV1& record) {
    if (storage_ == nullptr) {
        return {DeviceError::StorageError, "display layout store is unavailable"};
    }
    std::vector<uint8_t> blob;
    if (!encodeDisplayLayoutBinary(record, blob)) {
        return {DeviceError::InvalidConfig, "display layout payload is invalid"};
    }
    return storage_->saveBlob(record.deviceId, kDataType, blob.data(), blob.size());
}

bool DisplayLayoutStore::remove(DeviceId deviceId) {
    return storage_ != nullptr && storage_->remove(deviceId, kDataType);
}

bool DisplayLayoutStore::clearDevice(DeviceId deviceId) {
    return remove(deviceId);
}

} // namespace ewfm
