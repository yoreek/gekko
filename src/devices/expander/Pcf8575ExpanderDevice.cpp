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

Pcf8575ExpanderDevice::Pcf8575ExpanderDevice(const Pcf857xExpanderConfigV2& config) : Pcf857xExpanderDeviceBase(config) {}

Pcf8575ExpanderDevice::Pcf8575ExpanderDevice(const Pcf857xExpanderConfigV1& config) : Pcf857xExpanderDeviceBase(config) {}

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

uint8_t Pcf8575ExpanderDevice::channelCountImpl() const {
    return kPcf8575ChannelCount;
}

bool Pcf8575ExpanderDevice::writeChannelStates(II2cBusDriver& driver, uint32_t states) const {
    const uint8_t buffer[2] = {
        static_cast<uint8_t>(states & 0xFFU),
        static_cast<uint8_t>((states >> 8) & 0xFFU),
    };
    return driver.write(buffer, sizeof(buffer)) == sizeof(buffer);
}

} // namespace ewfm
