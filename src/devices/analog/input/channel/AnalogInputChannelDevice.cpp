#include "devices/analog/input/channel/AnalogInputChannelDevice.h"

namespace ewfm {

AnalogInputChannelDevice::AnalogInputChannelDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : AnalogInputChannelDevice([&configBlob]() {
          AnalogInputChannelDeviceConfigV1 config{};
          (void)decodeValidatedFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

AnalogInputChannelDevice::AnalogInputChannelDevice(const AnalogInputChannelDeviceConfigV1& config)
    : AnalogInputHubChannelDeviceBase((PState)&AnalogInputChannelDevice::Idle), config_(config) {}

const AnalogInputChannelDeviceConfigV1& AnalogInputChannelDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& AnalogInputChannelDevice::baseConfig() const {
    return config_;
}

void AnalogInputChannelDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

bool AnalogInputChannelDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = analogInputChannelDeviceConfigSize(config_);
    return encodeFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan AnalogInputChannelDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    AnalogInputChannelDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    const bool channelChanged = config.channel != config_.channel;
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = channelChanged;
    plan.resetStateMachine = channelChanged;
    return plan;
}

bool AnalogInputChannelDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    AnalogInputChannelDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

uint8_t AnalogInputChannelDevice::channel() const {
    return config_.channel;
}

uint8_t AnalogInputChannelDevice::adcSampleCount() const {
    return config_.poll.adcSamples;
}

bool AnalogInputChannelDevice::reportAlways() const {
    return config_.poll.reportAlways != 0U;
}

uint16_t AnalogInputChannelDevice::reportDeltaMilliVolts() const {
    return config_.poll.reportDeltaMilliVolts;
}

uint32_t AnalogInputChannelDevice::pollIntervalMs() const {
    return config_.poll.pollMs;
}

bool AnalogInputChannelDevice::channelEnabled() const {
    return config_.enabled != 0U;
}

DeviceTypeDescriptor AnalogInputChannelDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kAnalogInputChannelTypeId;
    descriptor.name = "AnalogInputChannelDevice";
    descriptor.currentConfigVersion = kAnalogInputChannelConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::AnalogInputHub, true}};
    descriptor.providedRoles = ProvidedRoles::of({IAnalogInputRuntime::kProvidedRole});
    descriptor.createRuntime = &AnalogInputChannelDevice::createRuntime;
    descriptor.validateConfig = &AnalogInputChannelDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> AnalogInputChannelDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                        const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new AnalogInputChannelDevice(record, configBlob));
}

DeviceValidationResult AnalogInputChannelDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::AnalogInputHub) == 0U) {
        return {DeviceError::InvalidRelationship, "analog input channel requires an analog input hub dependency"};
    }
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "analog input channel config exceeds supported size"};
    }
    AnalogInputChannelDeviceConfigV1 config{};
    if (!decodeValidatedFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "analog input channel config is invalid"};
    }
    return {};
}

} // namespace ewfm
