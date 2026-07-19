#include "integrations/mqtt/HaEntityAdapter.h"

#include "devices/analog/composer/AnalogOutputComposerDevice.h"
#include "devices/analog/fade/FadeAnalogOutputDevice.h"
#include "devices/analog/ledc/LedcAnalogOutputDevice.h"
#include "devices/analog/scheduled/ScheduledAnalogOutputDevice.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "devices/sensors/htu21/Htu21SensorDevice.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "integrations/mqtt/analog_output/AnalogOutputGroupModeHaEntityAdapter.h"
#include "integrations/mqtt/analog_output/AnalogOutputHaEntityAdapter.h"
#include "integrations/mqtt/analog_output/ScheduledAnalogOutputModeHaEntityAdapter.h"
#include "integrations/mqtt/binary/BinarySensorHaEntityAdapter.h"
#include "integrations/mqtt/dosing/DosingPumpHaEntityAdapters.h"
#include "integrations/mqtt/humidity/HumiditySensorHaEntityAdapter.h"
#include "integrations/mqtt/switch/SwitchOutputHaEntityAdapter.h"
#include "integrations/mqtt/temperature/TemperatureSensorHaEntityAdapter.h"
#include "integrations/mqtt/thermostat/ThermostatHaEntityAdapter.h"

#include <cstring>

namespace ewfm {

bool IHaEntityAdapter::applyCommand(DeviceRegistry& registry, const IDeviceRuntime& runtime, DeviceId deviceId,
                                    const std::string& commandKey, const std::string& payload, uint32_t now) const {
    (void)registry;
    (void)runtime;
    (void)deviceId;
    (void)commandKey;
    (void)payload;
    (void)now;
    return false;
}

bool HaEntityAdapterRegistry::registerAdapter(const IHaEntityAdapter& adapter) {
    if (adapter.typeId() == 0) {
        return false;
    }
    for (const auto* existing : adapters_) {
        if (existing != nullptr && existing->typeId() == adapter.typeId() && std::strcmp(existing->typeName(), adapter.typeName()) == 0) {
            return false;
        }
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
    static const SwitchOutputHaEntityAdapter kGpioSwitchAdapter(
        {GpioSwitchDevice::descriptor().typeId, "gpio_switch", "mdi:toggle-switch"});
    static const SwitchOutputHaEntityAdapter kPortExpanderSwitchAdapter(
        {PortExpanderSwitchDevice::descriptor().typeId, "port_expander_switch", "mdi:toggle-switch"});
    static const TemperatureSensorHaEntityAdapter kDs18b20Adapter(
        {Ds18b20TemperatureSensorDevice::descriptor().typeId, "ds18b20_temperature_sensor", "mdi:thermometer"});
    static const TemperatureSensorHaEntityAdapter kNtcThermistorAdapter(
        {NtcThermistorTemperatureSensorDevice::descriptor().typeId, "ntc_thermistor_temperature_sensor", "mdi:thermometer-lines"});
    // HTU21 exposes two independent HA entities from one device: temperature reuses the generic
    // sensor adapter above, humidity gets its own generic adapter below - HaEntityAdapterRegistry
    // allows more than one adapter per typeId precisely for cases like this.
    static const TemperatureSensorHaEntityAdapter kHtu21Adapter({Htu21SensorDevice::descriptor().typeId, "htu21", "mdi:thermometer"});
    static const HumiditySensorHaEntityAdapter kHtu21HumidityAdapter(
        {Htu21SensorDevice::descriptor().typeId, "htu21_humidity", "mdi:water-percent"});
    static const AnalogOutputHaEntityAdapter kLedcAnalogOutputAdapter(
        {LedcAnalogOutputDevice::descriptor().typeId, "analog_output", "mdi:brightness-6"});
    static const AnalogOutputHaEntityAdapter kFadeAnalogOutputAdapter(
        {FadeAnalogOutputDevice::descriptor().typeId, "fade_analog_output", "mdi:brightness-6"});
    static const AnalogOutputHaEntityAdapter kScheduledAnalogOutputAdapter(
        {ScheduledAnalogOutputDevice::descriptor().typeId, "scheduled_analog_output", "mdi:brightness-6"});
    // One dosing pump exposes five HA entities (four monitoring + auto-mode switch) - a single
    // Kind-parameterized adapter class instantiated per entity, same pattern as the generic
    // temperature/humidity sensor adapters above.
    static const DosingPumpHaEntityAdapter kDosingRunStateAdapter(
        {DosingPumpHaEntityKind::RunState, "dosing_pump_state", "dosing_state", "sensor", "State", "mdi:pump"});
    static const DosingPumpHaEntityAdapter kDosingTodayDosedAdapter(
        {DosingPumpHaEntityKind::TodayDosed, "dosing_pump_today_dosed", "dosing_today_dosed", "sensor", "Today dosed", "mdi:beaker-check"});
    static const DosingPumpHaEntityAdapter kDosingContainerLevelAdapter({DosingPumpHaEntityKind::ContainerLevel,
                                                                         "dosing_pump_container_level", "dosing_container_level", "sensor",
                                                                         "Container level", "mdi:cup-water"});
    static const DosingPumpHaEntityAdapter kDosingContainerEmptyAdapter({DosingPumpHaEntityKind::ContainerEmpty,
                                                                         "dosing_pump_container_empty", "dosing_container_empty",
                                                                         "binary_sensor", "Container empty", "mdi:cup-off-outline"});
    static const DosingPumpHaEntityAdapter kDosingAutoModeAdapter(
        {DosingPumpHaEntityKind::AutoMode, "dosing_pump_auto_mode", "dosing_auto_mode", "switch", "Auto mode", "mdi:calendar-sync"});

    HaEntityAdapterRegistry registry;
    (void)registry.registerAdapter(kGpioSwitchAdapter);
    (void)registry.registerAdapter(kDs18b20Adapter);
    (void)registry.registerAdapter(kNtcThermistorAdapter);
    (void)registry.registerAdapter(kHtu21Adapter);
    (void)registry.registerAdapter(kHtu21HumidityAdapter);
    (void)registry.registerAdapter(kLedcAnalogOutputAdapter);
    (void)registry.registerAdapter(kFadeAnalogOutputAdapter);
    (void)registry.registerAdapter(kScheduledAnalogOutputAdapter);
    (void)registry.registerAdapter(ScheduledAnalogOutputModeHaEntityAdapter::instance());
    (void)registry.registerAdapter(AnalogOutputGroupModeHaEntityAdapter::instance());
    (void)registry.registerAdapter(ThermostatHaEntityAdapter::instance());
    (void)registry.registerAdapter(kPortExpanderSwitchAdapter);
    (void)registry.registerAdapter(BinarySensorHaEntityAdapter::instance());
    (void)registry.registerAdapter(kDosingRunStateAdapter);
    (void)registry.registerAdapter(kDosingTodayDosedAdapter);
    (void)registry.registerAdapter(kDosingContainerLevelAdapter);
    (void)registry.registerAdapter(kDosingContainerEmptyAdapter);
    (void)registry.registerAdapter(kDosingAutoModeAdapter);
    return registry;
}

} // namespace ewfm
