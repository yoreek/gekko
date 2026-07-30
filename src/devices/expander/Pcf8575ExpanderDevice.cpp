#include "devices/expander/Pcf8575ExpanderDevice.h"

namespace ewfm {

namespace {
constexpr DeviceTypeId kPcf8575ExpanderTypeId = 13;
constexpr uint8_t kPcf8575ChannelCount = 16;
} // namespace

Pcf8575ExpanderDevice::Pcf8575ExpanderDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Pcf8575ExpanderDevice([&configBlob]() {
          Pcf857xExpanderConfigV2 config{};
          (void)decodePcf857xExpanderConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Pcf8575ExpanderDevice::Pcf8575ExpanderDevice(const Pcf857xExpanderConfigV2& config)
    : Pcf857xExpanderDeviceBase(config, kPcf8575ChannelCount) {}

DeviceTypeDescriptor Pcf8575ExpanderDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kPcf8575ExpanderTypeId;
    descriptor.name = "Pcf8575ExpanderDevice";
    descriptor.currentConfigVersion = kPcf857xExpanderConfigVersion;
    descriptor.maxDependents = kPcf8575ChannelCount;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks1s = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::I2CBus, true}};
    descriptor.providedRoles = ProvidedRoles::of({IPortExpanderRuntime::kProvidedRole});
    descriptor.createRuntime = &Pcf8575ExpanderDevice::createRuntime;
    descriptor.validateConfig = &Pcf857xExpanderDeviceBase::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Pcf8575ExpanderDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                     const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Pcf8575ExpanderDevice(record, configBlob));
}

} // namespace ewfm
