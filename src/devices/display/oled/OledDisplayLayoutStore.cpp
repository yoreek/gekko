#include "devices/display/oled/OledDisplayLayoutStore.h"

#include "devices/display/oled/OledDisplayLayoutCodec.h"

namespace ewfm {

bool OledDisplayLayoutStore::begin(bool readOnly) {
    return storage_ != nullptr && storage_->begin(readOnly);
}

DeviceValidationResult OledDisplayLayoutStore::load(DeviceId deviceId, OledDisplayLayoutRecordV1& record) {
    std::vector<uint8_t> blob;
    if (storage_ == nullptr) {
        return {DeviceError::StorageError, "failed to open device scoped storage"};
    }
    const DeviceValidationResult result = storage_->loadBlob(deviceId, kDataType, blob);
    if (!result.ok()) {
        return result;
    }
    if (!decodeOledDisplayLayoutBinary(blob.data(), blob.size(), record)) {
        (void)storage_->remove(deviceId, kDataType);
        return {DeviceError::MissingRecord, "device scoped data is missing"};
    }
    if (record.deviceId != deviceId) {
        (void)storage_->remove(deviceId, kDataType);
        return {DeviceError::InvalidDeviceId, "device scoped data device id is invalid"};
    }
    return {};
}

DeviceValidationResult OledDisplayLayoutStore::save(const OledDisplayLayoutRecordV1& record) {
    if (storage_ == nullptr) {
        return {DeviceError::StorageError, "failed to open device scoped storage"};
    }
    std::vector<uint8_t> blob;
    if (!encodeOledDisplayLayoutBinary(record, blob)) {
        return {DeviceError::InvalidConfig, "oled display layout is invalid"};
    }
    return storage_->saveBlob(record.deviceId, kDataType, blob.data(), blob.size());
}

bool OledDisplayLayoutStore::remove(DeviceId deviceId) {
    return storage_ != nullptr && storage_->remove(deviceId, kDataType);
}

bool OledDisplayLayoutStore::clearDevice(DeviceId deviceId) {
    return storage_ != nullptr && storage_->clearDevice(deviceId);
}

} // namespace ewfm
