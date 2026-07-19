#include "devices/analog/fade/FadeAnalogOutputDevice.h"

#include "devices/core/ConfigCodec.h"

#include <algorithm>

namespace ewfm {

FadeAnalogOutputDevice::FadeAnalogOutputDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : FadeAnalogOutputDevice([&configBlob]() {
          FadeAnalogOutputDeviceConfigV1 config{};
          (void)decodeFadeAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

FadeAnalogOutputDevice::FadeAnalogOutputDevice(const FadeAnalogOutputDeviceConfigV1& config) : config_(config) {}

const FadeAnalogOutputDeviceConfigV1& FadeAnalogOutputDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& FadeAnalogOutputDevice::baseConfig() const {
    return config_;
}

uint16_t FadeAnalogOutputDevice::targetOutputState() const {
    return targetState_;
}

bool FadeAnalogOutputDevice::transitioning() const {
    return targetOutput() != nullptr && currentOutputState() != targetState_;
}

bool FadeAnalogOutputDevice::requestOutputState(const uint16_t state, const uint32_t now) {
    if (state > kAnalogOutputLevelMax || status() != DeviceStatus::Ready) {
        return false;
    }
    targetState_ = state;
    targetInitialized_ = true;
    if (lastStepAt_ == 0U) {
        lastStepAt_ = now;
    }
    markRuntimeStateDirty();
    return true;
}

bool FadeAnalogOutputDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = fadeAnalogOutputDeviceConfigSize(config_);
    return encodeFixedConfigBlob(FadeAnalogOutputDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan FadeAnalogOutputDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    FadeAnalogOutputDeviceConfigV1 next{};
    (void)decodeFadeAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), next);
    return {};
}

bool FadeAnalogOutputDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    FadeAnalogOutputDeviceConfigV1 next{};
    if (!decodeFadeAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return false;
    }
    config_ = next;
    lastStepAt_ = now;
    return true;
}

DeviceTypeDescriptor FadeAnalogOutputDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kFadeAnalogOutputDeviceTypeId;
    descriptor.name = "FadeAnalogOutputDevice";
    descriptor.currentConfigVersion = kFadeAnalogOutputDeviceConfigVersion;
    descriptor.maxDependents = 8;
    descriptor.supportsCommands = true;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::AnalogOutput, true}};
    descriptor.exclusiveDependencyRoles = ProvidedRoles::of({DeviceRole::AnalogOutput});
    descriptor.providedRoles = ProvidedRoles::of({IAnalogOutputRuntime::kProvidedRole});
    descriptor.createRuntime = &FadeAnalogOutputDevice::createRuntime;
    descriptor.validateConfig = &FadeAnalogOutputDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> FadeAnalogOutputDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                      const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new FadeAnalogOutputDevice(record, configBlob));
}

DeviceValidationResult FadeAnalogOutputDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::AnalogOutput) == 0U) {
        return {DeviceError::InvalidRelationship, "analog output dependency is required"};
    }
    FadeAnalogOutputDeviceConfigV1 config{};
    if (!decodeFadeAnalogOutputDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "fade analog output config is invalid"};
    }
    return config.validate();
}

void FadeAnalogOutputDevice::onTargetAttached(const uint32_t now) {
    if (!targetInitialized_) {
        targetState_ = currentOutputState();
        targetInitialized_ = true;
    }
    lastStepAt_ = now;
}

void FadeAnalogOutputDevice::onReadyTick(const uint32_t now) {
    const uint16_t current = currentOutputState();
    if (current == targetState_) {
        lastStepAt_ = now;
        return;
    }
    const uint32_t elapsed = now - lastStepAt_;
    const uint32_t intervals = elapsed / config_.stepIntervalMs;
    if (intervals == 0U) {
        return;
    }
    lastStepAt_ += intervals * config_.stepIntervalMs;
    const uint32_t availableStep =
        std::min<uint32_t>(static_cast<uint32_t>(kAnalogOutputLevelMax), intervals * static_cast<uint32_t>(config_.maxStep));
    uint16_t next = current;
    if (current < targetState_) {
        next = static_cast<uint16_t>(current + std::min<uint32_t>(availableStep, targetState_ - current));
    } else {
        next = static_cast<uint16_t>(current - std::min<uint32_t>(availableStep, current - targetState_));
    }
    if (forwardOutputState(next, now)) {
        markRuntimeStateDirty();
    }
}

} // namespace ewfm
