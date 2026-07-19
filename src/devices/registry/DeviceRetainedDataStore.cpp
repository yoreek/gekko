#include "devices/registry/DeviceRetainedDataStore.h"

#include <cstdio>

namespace ewfm {

DeviceRetainedDataStore::DeviceRetainedDataStore(IConfigStorage& storage) : storage_(storage) {}

bool DeviceRetainedDataStore::begin(bool readOnly) {
    return storage_.begin(readOnly);
}

bool DeviceRetainedDataStore::remove(DeviceId deviceId) {
    return storage_.remove(deviceId, kRetainedStateDataType);
}

bool DeviceRetainedDataStore::clearDevice(DeviceId deviceId) {
    return storage_.clearDevice(deviceId);
}

} // namespace ewfm
