#include "devices/registry/DeviceScopedDataStore.h"

#include <cstdio>

namespace ewfm {

namespace {
constexpr const char* kNamespacePrefix = "dev_";
}

DeviceScopedDataStore::DeviceScopedDataStore(IConfigStorage& storage) : storage_(storage) {}

bool DeviceScopedDataStore::begin(bool readOnly) {
    (void)readOnly;
    return true;
}

DeviceValidationResult DeviceScopedDataStore::loadBlob(DeviceId deviceId, const char* dataType, std::vector<uint8_t>& blob) {
    blob.clear();
    bool opened = false;
    if (!openDeviceNamespace(deviceId, true, opened)) {
        return {DeviceError::MissingRecord, "device scoped data is missing"};
    }
    const std::string key = makeDataKey(dataType);
    if (!storage_.getBlob(key.c_str(), blob) || blob.empty()) {
        if (opened) {
            storage_.end();
        }
        return {DeviceError::MissingRecord, "device scoped data is missing"};
    }
    if (opened) {
        storage_.end();
    }
    return {};
}

DeviceValidationResult DeviceScopedDataStore::saveBlob(DeviceId deviceId, const char* dataType, const uint8_t* data, size_t size) {
    if (deviceId == 0U) {
        return {DeviceError::InvalidDeviceId, "device scoped data device id is invalid"};
    }
    bool opened = false;
    if (!openDeviceNamespace(deviceId, false, opened)) {
        return {DeviceError::StorageError, "failed to open device scoped storage"};
    }
    const std::string key = makeDataKey(dataType);
    const bool ok = storage_.putBlob(key.c_str(), data, size);
    if (opened) {
        storage_.end();
    }
    return ok ? DeviceValidationResult{} : DeviceValidationResult{DeviceError::StorageError, "failed to persist device scoped data"};
}

bool DeviceScopedDataStore::remove(DeviceId deviceId, const char* dataType) {
    bool opened = false;
    if (!openDeviceNamespace(deviceId, false, opened)) {
        return false;
    }
    const std::string key = makeDataKey(dataType);
    const bool removed = storage_.remove(key.c_str());
    if (opened) {
        storage_.end();
    }
    return removed;
}

bool DeviceScopedDataStore::clearDevice(DeviceId deviceId) {
    bool opened = false;
    if (!openDeviceNamespace(deviceId, false, opened)) {
        return false;
    }
    const bool cleared = storage_.clear();
    if (opened) {
        storage_.end();
    }
    return cleared;
}

bool DeviceScopedDataStore::makeNamespace(char* buffer, size_t bufferSize, DeviceId deviceId) {
    return std::snprintf(buffer, bufferSize, "%s%08x", kNamespacePrefix, static_cast<unsigned>(deviceId)) >= 0;
}

std::string DeviceScopedDataStore::makeDataKey(const char* dataType) {
    return dataType == nullptr ? std::string{} : std::string(dataType);
}

bool DeviceScopedDataStore::openDeviceNamespace(DeviceId deviceId, bool readOnly, bool& opened) {
    char namespaceName[16];
    if (!makeNamespace(namespaceName, sizeof(namespaceName), deviceId)) {
        return false;
    }
    opened = storage_.begin(namespaceName, readOnly);
    return opened;
}

} // namespace ewfm
