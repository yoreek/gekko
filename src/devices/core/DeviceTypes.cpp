#include "devices/core/DeviceTypes.h"

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/display/ssd1306/Ssd1306Device.h"
#include "devices/display/st7735/St7735Device.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "devices/thermostat/ThermostatDevice.h"

namespace ewfm {

namespace {
constexpr const char* kDeviceRoleNames[] = {
    "unknown", "onewire_bus", "temperature_sensor", "switch", "i2c_bus", "ssd1306", "spi_bus", "metric_source",
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
    role = DeviceRole::Unknown;
    return false;
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
    (void)registry.registerDescriptor(ThermostatDevice::descriptor());
    return registry;
}

} // namespace ewfm
