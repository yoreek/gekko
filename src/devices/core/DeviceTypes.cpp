#include "devices/core/DeviceTypes.h"

#include "devices/analog/composer/AnalogOutputComposerDevice.h"
#include "devices/analog/fade/FadeAnalogOutputDevice.h"
#include "devices/analog/input/ads1115/Ads1115HubDevice.h"
#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDevice.h"
#include "devices/analog/input/channel/AnalogInputChannelDevice.h"
#include "devices/analog/input/port/AnalogPortInputDevice.h"
#include "devices/analog/ledc/LedcAnalogOutputDevice.h"
#include "devices/analog/scheduled/ScheduledAnalogOutputDevice.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/display/lcd1602/Lcd1602Device.h"
#include "devices/display/lcd2004/Lcd2004Device.h"
#include "devices/display/ssd1306/Ssd1306Device.h"
#include "devices/display/st7735/St7735Device.h"
#include "devices/dosing/DosingPumpDevice.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/expander/Pcf8574ExpanderDevice.h"
#include "devices/expander/Pcf8575ExpanderDevice.h"
#include "devices/rtc/ds3231/Ds3231RtcDevice.h"
#include "devices/schedule/ScheduleDevice.h"
#include "devices/sensors/aht10/Aht10SensorDevice.h"
#include "devices/sensors/binary/BinarySensorDevice.h"
#include "devices/sensors/dht11/Dht11SensorDevice.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "devices/sensors/htu21/Htu21SensorDevice.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "devices/switch/auto/AutoSwitchDevice.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "devices/thermostat/ThermostatDevice.h"

namespace ewfm {

namespace {
constexpr const char* kDeviceRoleNames[] = {
    "unknown",       "onewire_bus",         "temperature_sensor", "switch",           "i2c_bus",  "ssd1306",
    "spi_bus",       "metric_source",       "real_time_clock",    "port_expander",    "schedule", "condition",
    "analog_output", "analog_output_group", "analog_input",       "analog_input_hub",
};

constexpr const char* kDeviceEventKindNames[] = {
    "registry_loaded",
    "device_created",
    "device_updated",
    "device_deleted",
    "status_changed",
    "state_changed",
    "command_accepted",
    "command_rejected",
    "config_persisted",
    "retained_state_changed",
    "persistence_pending_cleared",
};
} // namespace

const char* deviceRoleName(DeviceRole role) {
    const auto index = static_cast<size_t>(role);
    if (index >= (sizeof(kDeviceRoleNames) / sizeof(kDeviceRoleNames[0]))) {
        return kDeviceRoleNames[0];
    }
    return kDeviceRoleNames[index];
}

bool parseDeviceRole(std::string_view value, DeviceRole& role) {
    if (value == "onewire_bus") {
        role = DeviceRole::OneWireBus;
        return true;
    }
    if (value == "temperature_sensor") {
        role = DeviceRole::TemperatureSensor;
        return true;
    }
    if (value == "switch") {
        role = DeviceRole::Switch;
        return true;
    }
    if (value == "i2c_bus") {
        role = DeviceRole::I2CBus;
        return true;
    }
    if (value == "ssd1306") {
        role = DeviceRole::Ssd1306;
        return true;
    }
    if (value == "spi_bus") {
        role = DeviceRole::SpiBus;
        return true;
    }
    if (value == "metric_source") {
        role = DeviceRole::MetricSource;
        return true;
    }
    if (value == "real_time_clock") {
        role = DeviceRole::RealTimeClock;
        return true;
    }
    if (value == "port_expander") {
        role = DeviceRole::PortExpander;
        return true;
    }
    if (value == "schedule") {
        role = DeviceRole::Schedule;
        return true;
    }
    if (value == "condition") {
        role = DeviceRole::Condition;
        return true;
    }
    if (value == "analog_output") {
        role = DeviceRole::AnalogOutput;
        return true;
    }
    if (value == "analog_output_group") {
        role = DeviceRole::AnalogOutputGroup;
        return true;
    }
    if (value == "analog_input") {
        role = DeviceRole::AnalogInput;
        return true;
    }
    if (value == "analog_input_hub") {
        role = DeviceRole::AnalogInputHub;
        return true;
    }
    role = DeviceRole::Unknown;
    return false;
}

const char* analogOutputModeName(const AnalogOutputMode mode) {
    switch (mode) {
    case AnalogOutputMode::Off:
        return "off";
    case AnalogOutputMode::Manual:
        return "manual";
    case AnalogOutputMode::Scheduled:
        return "scheduled";
    }
    return "off";
}

const char* deviceEventKindName(const DeviceEventKind kind) {
    const auto index = static_cast<size_t>(kind);
    if (index >= (sizeof(kDeviceEventKindNames) / sizeof(kDeviceEventKindNames[0]))) {
        return kDeviceEventKindNames[0];
    }
    return kDeviceEventKindNames[index];
}

bool DeviceTypeRegistry::registerDescriptor(const DeviceTypeDescriptor& descriptor) {
    if (descriptor.typeId == 0 || descriptor.name == nullptr) {
        return false;
    }

    if (find(descriptor.typeId) != nullptr) {
        return false;
    }

    descriptors_.push_back(descriptor);
    return true;
}

const DeviceTypeDescriptor* DeviceTypeRegistry::find(DeviceTypeId typeId) const {
    for (const auto& descriptor : descriptors_) {
        if (descriptor.typeId == typeId) {
            return &descriptor;
        }
    }
    return nullptr;
}

DeviceTypeRegistry DeviceTypeRegistry::withDefaults() {
    DeviceTypeRegistry registry;
    (void)registry.registerDescriptor(DummyDevice::descriptor());
    (void)registry.registerDescriptor(GpioSwitchDevice::descriptor());
    (void)registry.registerDescriptor(OneWireBusDevice::descriptor());
    (void)registry.registerDescriptor(I2cBusDevice::descriptor());
    (void)registry.registerDescriptor(SpiBusDevice::descriptor());
    (void)registry.registerDescriptor(Ssd1306Device::descriptor());
    (void)registry.registerDescriptor(St7735Device::descriptor());
    (void)registry.registerDescriptor(Ds18b20TemperatureSensorDevice::descriptor());
    (void)registry.registerDescriptor(NtcThermistorTemperatureSensorDevice::descriptor());
    (void)registry.registerDescriptor(Dht11SensorDevice::descriptor());
    (void)registry.registerDescriptor(Htu21SensorDevice::descriptor());
    (void)registry.registerDescriptor(Aht10SensorDevice::descriptor());
    (void)registry.registerDescriptor(ThermostatDevice::descriptor());
    (void)registry.registerDescriptor(Ds3231RtcDevice::descriptor());
    (void)registry.registerDescriptor(Pcf8574ExpanderDevice::descriptor());
    (void)registry.registerDescriptor(Pcf8575ExpanderDevice::descriptor());
    (void)registry.registerDescriptor(LedcAnalogOutputDevice::descriptor());
    (void)registry.registerDescriptor(FadeAnalogOutputDevice::descriptor());
    (void)registry.registerDescriptor(ScheduledAnalogOutputDevice::descriptor());
    (void)registry.registerDescriptor(AnalogOutputComposerDevice::descriptor());
    (void)registry.registerDescriptor(PortExpanderSwitchDevice::descriptor());
    (void)registry.registerDescriptor(ScheduleDevice::descriptor());
    (void)registry.registerDescriptor(AutoSwitchDevice::descriptor());
    (void)registry.registerDescriptor(BinarySensorDevice::descriptor());
    (void)registry.registerDescriptor(DosingPumpDevice::descriptor());
    (void)registry.registerDescriptor(AnalogPortInputDevice::descriptor());
    (void)registry.registerDescriptor(Cd74hc4067HubDevice::descriptor());
    (void)registry.registerDescriptor(Ads1115HubDevice::descriptor());
    (void)registry.registerDescriptor(AnalogInputChannelDevice::descriptor());
    (void)registry.registerDescriptor(Lcd1602Device::descriptor());
    (void)registry.registerDescriptor(Lcd2004Device::descriptor());
    return registry;
}

} // namespace ewfm
