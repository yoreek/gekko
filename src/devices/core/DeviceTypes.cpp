#include "devices/core/DeviceTypes.h"

#include "devices/bus/onewire/OneWireBusDevice.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "devices/thermostat/ThermostatDevice.h"

namespace ewfm {

namespace {
constexpr const char* kDeviceDependencyRoleNames[] = {
    "unknown",
    "onewire_bus",
    "temperature_sensor",
    "switch",
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

const char* deviceDependencyRoleName(DeviceDependencyRole role) {
    const auto index = static_cast<size_t>(role);
    if (index >= (sizeof(kDeviceDependencyRoleNames) / sizeof(kDeviceDependencyRoleNames[0]))) {
        return kDeviceDependencyRoleNames[0];
    }
    return kDeviceDependencyRoleNames[index];
}

bool parseDeviceDependencyRole(std::string_view value, DeviceDependencyRole& role) {
    if (value == "onewire_bus") {
        role = DeviceDependencyRole::OneWireBus;
        return true;
    }
    if (value == "temperature_sensor") {
        role = DeviceDependencyRole::TemperatureSensor;
        return true;
    }
    if (value == "switch") {
        role = DeviceDependencyRole::Switch;
        return true;
    }
    role = DeviceDependencyRole::Unknown;
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
    (void)registry.registerDescriptor(Ds18b20TemperatureSensorDevice::descriptor());
    (void)registry.registerDescriptor(ThermostatDevice::descriptor());
    return registry;
}

} // namespace ewfm
