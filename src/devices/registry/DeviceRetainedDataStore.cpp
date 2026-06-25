#include "devices/registry/DeviceRetainedDataStore.h"

#include <cstdio>

namespace ewfm {

DeviceRetainedDataStore::DeviceRetainedDataStore(IConfigStorage& storage) : legacyStorage_(storage), storage_(storage) {}

bool DeviceRetainedDataStore::begin(bool readOnly) {
    return storage_.begin(readOnly);
}

bool DeviceRetainedDataStore::remove(DeviceId deviceId) {
    bool ok = storage_.remove(deviceId, kRetainedStateDataType);
    if (legacyStorage_.begin(kLegacyNamespace, false)) {
        char key[32];
        if (makeLegacyKey(key, sizeof(key), deviceId)) {
            ok = legacyStorage_.remove(key) && ok;
        }
        legacyStorage_.end();
    }
    return ok;
}

bool DeviceRetainedDataStore::clearDevice(DeviceId deviceId) {
    bool ok = storage_.clearDevice(deviceId);
    if (legacyStorage_.begin(kLegacyNamespace, false)) {
        char key[32];
        if (makeLegacyKey(key, sizeof(key), deviceId)) {
            ok = legacyStorage_.remove(key) && ok;
        }
        legacyStorage_.end();
    }
    return ok;
}

bool DeviceRetainedDataStore::clearLegacyNamespace() {
    if (!legacyStorage_.begin(kLegacyNamespace, false)) {
        return false;
    }
    const bool cleared = legacyStorage_.clear();
    legacyStorage_.end();
    return cleared;
}

bool DeviceRetainedDataStore::makeLegacyKey(char* buffer, size_t bufferSize, DeviceId deviceId) {
    return std::snprintf(buffer, bufferSize, "%s%08x", kLegacyRetainedStateKeyPrefix, static_cast<unsigned>(deviceId)) >= 0;
}

} // namespace ewfm
