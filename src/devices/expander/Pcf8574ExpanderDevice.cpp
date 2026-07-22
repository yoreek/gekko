#include "devices/expander/Pcf8574ExpanderDevice.h"

namespace ewfm {

namespace {
constexpr DeviceTypeId kPcf8574ExpanderTypeId = 12;
constexpr uint8_t kPcf8574ChannelCount = 8;
} // namespace

Pcf8574ExpanderDevice::Pcf8574ExpanderDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Pcf8574ExpanderDevice([&configBlob]() {
          Pcf857xExpanderConfigV2 config{};
          (void)decodePcf857xExpanderConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

Pcf8574ExpanderDevice::Pcf8574ExpanderDevice(const Pcf857xExpanderConfigV2& config) : Pcf857xExpanderDeviceBase(config) {}

DeviceTypeDescriptor Pcf8574ExpanderDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kPcf8574ExpanderTypeId;
    descriptor.name = "Pcf8574ExpanderDevice";
    descriptor.currentConfigVersion = kPcf857xExpanderConfigVersion;
    descriptor.maxDependents = kPcf8574ChannelCount;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks1s = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::I2CBus, true}};
    descriptor.providedRoles = ProvidedRoles::of({IPortExpanderRuntime::kProvidedRole});
    descriptor.createRuntime = &Pcf8574ExpanderDevice::createRuntime;
    descriptor.validateConfig = &Pcf857xExpanderDeviceBase::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Pcf8574ExpanderDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                     const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Pcf8574ExpanderDevice(record, configBlob));
}

uint8_t Pcf8574ExpanderDevice::channelCountImpl() const {
    return kPcf8574ChannelCount;
}

bool Pcf8574ExpanderDevice::writeChannelStates(II2cBusDriver& driver, uint32_t states) const {
    return driver.write(static_cast<uint8_t>(states & 0xFFU)) == 1U;
}

} // namespace ewfm
