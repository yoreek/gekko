#include "devices/registry/RetainedStateStore.h"

#include <cstdio>

namespace ewfm {

namespace {
constexpr const char* kNamespace = "device_retained";
} // namespace

RetainedStateStore::RetainedStateStore(IConfigStorage& storage) : storage_(storage) {}

bool RetainedStateStore::begin(bool readOnly) {
    return storage_.begin(kNamespace, readOnly);
}

bool RetainedStateStore::remove(DeviceId deviceId) {
    char key[32];
    return makeStateKey(key, sizeof(key), deviceId) && storage_.remove(key);
}

bool RetainedStateStore::makeStateKey(char* buffer, size_t bufferSize, DeviceId deviceId) {
    return std::snprintf(buffer, bufferSize, "state_%08x", static_cast<unsigned>(deviceId)) >= 0;
}

} // namespace ewfm
