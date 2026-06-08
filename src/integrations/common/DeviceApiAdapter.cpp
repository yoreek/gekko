#include "integrations/common/DeviceApiAdapter.h"

#include "integrations/rest/dummy/DummyDeviceApiAdapter.h"

#include <cstring>

namespace ewfm {

bool DeviceApiAdapterRegistry::registerAdapter(const IDeviceApiAdapter& adapter) {
    if (adapter.typeId() == 0 || adapter.typeName() == nullptr) {
        return false;
    }
    if (find(adapter.typeId()) != nullptr || findByName(adapter.typeName()) != nullptr) {
        return false;
    }
    adapters_.push_back(&adapter);
    return true;
}

const IDeviceApiAdapter* DeviceApiAdapterRegistry::find(DeviceTypeId typeId) const {
    for (const auto* adapter : adapters_) {
        if (adapter != nullptr && adapter->typeId() == typeId) {
            return adapter;
        }
    }
    return nullptr;
}

const IDeviceApiAdapter* DeviceApiAdapterRegistry::findByName(const char* name) const {
    if (name == nullptr) {
        return nullptr;
    }
    for (const auto* adapter : adapters_) {
        if (adapter != nullptr && adapter->typeName() != nullptr && std::strcmp(adapter->typeName(), name) == 0) {
            return adapter;
        }
    }
    return nullptr;
}

DeviceApiAdapterRegistry DeviceApiAdapterRegistry::withDefaults() {
    DeviceApiAdapterRegistry registry;
    (void)registry.registerAdapter(DummyDeviceApiAdapter::instance());
    return registry;
}

} // namespace ewfm
