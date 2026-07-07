#include "integrations/mqtt/HaEntityAdapter.h"

#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "integrations/mqtt/gpio_switch/GpioSwitchHaEntityAdapter.h"
#include "integrations/mqtt/temperature/TemperatureSensorHaEntityAdapter.h"
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
    static const TemperatureSensorHaEntityAdapter kDs18b20Adapter(
        {Ds18b20TemperatureSensorDevice::descriptor().typeId, "ds18b20_temperature_sensor", "mdi:thermometer"});
    static const TemperatureSensorHaEntityAdapter kNtcThermistorAdapter(
        {NtcThermistorTemperatureSensorDevice::descriptor().typeId, "ntc_thermistor_temperature_sensor", "mdi:thermometer-lines"});

    HaEntityAdapterRegistry registry;
    (void)registry.registerAdapter(GpioSwitchHaEntityAdapter::instance());
    (void)registry.registerAdapter(kDs18b20Adapter);
    (void)registry.registerAdapter(kNtcThermistorAdapter);
    (void)registry.registerAdapter(ThermostatHaEntityAdapter::instance());
    return registry;
}

} // namespace ewfm
