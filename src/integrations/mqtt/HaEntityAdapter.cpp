#include "integrations/mqtt/HaEntityAdapter.h"

#include "integrations/mqtt/ds18b20/Ds18b20HaEntityAdapter.h"
#include "integrations/mqtt/gpio_switch/GpioSwitchHaEntityAdapter.h"
#include "integrations/mqtt/thermostat/ThermostatHaEntityAdapter.h"

namespace ewfm {

bool HaEntityAdapterRegistry::registerAdapter(const IHaEntityAdapter& adapter) {
    if (adapter.typeId() == 0 || find(adapter.typeId()) != nullptr) {
        return false;
    }
    adapters_.push_back(&adapter);
    return true;
}

const IHaEntityAdapter* HaEntityAdapterRegistry::find(DeviceTypeId typeId) const {
    for (const auto* adapter : adapters_) {
        if (adapter != nullptr && adapter->typeId() == typeId) {
            return adapter;
        }
    }
    return nullptr;
}

HaEntityAdapterRegistry HaEntityAdapterRegistry::withDefaults() {
    HaEntityAdapterRegistry registry;
    (void)registry.registerAdapter(GpioSwitchHaEntityAdapter::instance());
    (void)registry.registerAdapter(Ds18b20HaEntityAdapter::instance());
    (void)registry.registerAdapter(ThermostatHaEntityAdapter::instance());
    return registry;
}

} // namespace ewfm
