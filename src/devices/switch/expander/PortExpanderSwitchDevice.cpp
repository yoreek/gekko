#include "devices/switch/expander/PortExpanderSwitchDevice.h"

#include "devices/core/ConfigCodec.h"

#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kPortExpanderSwitchDeviceTypeId = 14;
constexpr uint32_t kPortExpanderSwitchDeviceConfigVersion = 3;

} // namespace

PortExpanderSwitchDevice::PortExpanderSwitchDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : PortExpanderSwitchDevice([&configBlob]() {
          PortExpanderSwitchDeviceConfigV3 config{};
          (void)decodePortExpanderSwitchDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

PortExpanderSwitchDevice::PortExpanderSwitchDevice(const PortExpanderSwitchDeviceConfigV3& config)
    : SwitchDeviceBase(config), config_(config) {}

uint8_t PortExpanderSwitchDevice::channel() const {
    return config_.channel;
}

const PortExpanderSwitchDeviceConfigV3& PortExpanderSwitchDevice::config() const {
    return config_;
}

bool PortExpanderSwitchDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = portExpanderSwitchDeviceConfigSize(config_);
    return encodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan PortExpanderSwitchDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    PortExpanderSwitchDeviceConfigV3 config{};
    if (!decodePortExpanderSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool invertedChanged = config.inverted != config_.inverted;
    const bool channelChanged = config.channel != config_.channel;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = channelChanged || invertedChanged;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool PortExpanderSwitchDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    PortExpanderSwitchDeviceConfigV3 config{};
    if (!decodePortExpanderSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

SwitchDeviceConfigV2& PortExpanderSwitchDevice::mutableConfig() {
    return config_;
}

bool PortExpanderSwitchDevice::expanderChannel(uint8_t& channel) const {
    channel = config_.channel;
    return true;
}
DeviceTypeDescriptor PortExpanderSwitchDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kPortExpanderSwitchDeviceTypeId;
    descriptor.name = "PortExpanderSwitchDevice";
    descriptor.currentConfigVersion = kPortExpanderSwitchDeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticksFastLoop = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::PortExpander, true}};
    descriptor.providedRoles = ProvidedRoles::of({ISwitchOutputRuntime::kProvidedRole, IStatusRuntime::kProvidedRole});
    descriptor.createRuntime = &PortExpanderSwitchDevice::createRuntime;
    descriptor.validateConfig = &PortExpanderSwitchDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> PortExpanderSwitchDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                        const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new PortExpanderSwitchDevice(record, configBlob));
}

DeviceValidationResult PortExpanderSwitchDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::PortExpander) == 0U) {
        return {DeviceError::InvalidRelationship, "port expander switch requires a port expander dependency"};
    }
    return validateOutputConfig<PortExpanderSwitchDeviceConfigV3>(configBlob, decodePortExpanderSwitchDeviceConfig);
}

DeviceValidationResult PortExpanderSwitchDevice::configureHardware(uint32_t now) {
    (void)now;
    const IPortExpanderRuntime* expander = dependencyExpander();
    if (expander == nullptr) {
        return {DeviceError::InvalidRelationship, "port expander switch dependency is not ready"};
    }
    if (config_.channel >= expander->channelCount()) {
        return {DeviceError::InvalidConfig, "port expander switch channel is out of range"};
    }
    return {};
}

DeviceValidationResult PortExpanderSwitchDevice::applyHardwareOutput(const bool state, const uint32_t now) {
    IPortExpanderRuntime* expander = dependencyExpander();
    if (expander == nullptr) {
        return {DeviceError::InvalidRelationship, "port expander switch dependency is not ready"};
    }

    return expander->requestChannelState(config_.channel, state, now)
               ? DeviceValidationResult{}
               : DeviceValidationResult{DeviceError::StorageError, "port expander channel write failed"};
}

void PortExpanderSwitchDevice::releaseHardware(uint32_t now) {
    (void)now;
}

IPortExpanderRuntime* PortExpanderSwitchDevice::dependencyExpander() const {
    IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::PortExpander);
    if (dependency == nullptr) {
        return nullptr;
    }
    return const_cast<IPortExpanderRuntime*>(dependency->portExpanderRuntime());
}

} // namespace ewfm
