#include "devices/pixel/effects/PixelEffectAlertDevice.h"

#include "devices/core/ConfigCodec.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS PixelEffectAlertDevice

PixelEffectAlertDevice::PixelEffectAlertDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : PixelEffectAlertDevice([&configBlob]() {
          PixelEffectAlertDeviceConfigV1 config{};
          (void)decodePixelEffectAlertDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

PixelEffectAlertDevice::PixelEffectAlertDevice(const PixelEffectAlertDeviceConfigV1& config)
    : DeviceRuntimeBase((PState)&PixelEffectAlertDevice::Idle), config_(config) {}

const PixelEffectAlertDeviceConfigV1& PixelEffectAlertDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& PixelEffectAlertDevice::baseConfig() const {
    return config_;
}

void PixelEffectAlertDevice::setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntime(role, dependencyRuntime);
    refreshCapabilityCache();
}

void PixelEffectAlertDevice::setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntimeAt(index, dependencyRuntime);
    refreshCapabilityCache();
}

void PixelEffectAlertDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
    refreshCapabilityCache();
}

bool PixelEffectAlertDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = pixelEffectAlertDeviceConfigSize(config_);
    return encodeFixedConfigBlob(PixelEffectAlertDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan PixelEffectAlertDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    PixelEffectAlertDeviceConfigV1 next{};
    (void)decodePixelEffectAlertDeviceConfig(configBlob.data(), configBlob.size(), next);
    return {};
}

bool PixelEffectAlertDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    PixelEffectAlertDeviceConfigV1 next{};
    if (!decodePixelEffectAlertDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return false;
    }
    config_ = next;
    hasApplied_ = false;
    return true;
}

bool PixelEffectAlertDevice::conditionsSatisfied() const {
    if (conditionCount_ == 0U) {
        return false;
    }
    for (uint8_t index = 0; index < conditionCount_; ++index) {
        const ConditionSource& condition = conditions_[index];
        const bool active = condition.source != nullptr && condition.source->isActive();
        if (active == condition.invert) {
            return false;
        }
    }
    return true;
}

DeviceTypeDescriptor PixelEffectAlertDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kPixelEffectAlertDeviceTypeId;
    descriptor.name = "PixelEffectAlertDevice";
    descriptor.currentConfigVersion = kPixelEffectAlertDeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = false;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::PixelStrip, true},
                                         DeviceDependencyRequirement{DeviceRole::Condition, false}};
    descriptor.exclusiveDependencyRoles = ProvidedRoles::of({DeviceRole::PixelStrip});
    descriptor.createRuntime = &PixelEffectAlertDevice::createRuntime;
    descriptor.validateConfig = &PixelEffectAlertDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> PixelEffectAlertDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                      const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new PixelEffectAlertDevice(record, configBlob));
}

DeviceValidationResult PixelEffectAlertDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::PixelStrip) == 0U) {
        return {DeviceError::InvalidRelationship, "pixel strip dependency is required"};
    }
    PixelEffectAlertDeviceConfigV1 config{};
    if (!decodePixelEffectAlertDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "pixel effect alert config is invalid"};
    }
    return config.validate();
}

void PixelEffectAlertDevice::refreshCapabilityCache() {
    targetStrip_ = nullptr;
    conditions_ = {};
    conditionCount_ = 0U;
    const DeviceDependencyLink* links = dependencyLinks();
    const uint8_t count = dependencyCount();
    for (uint8_t index = 0; index < count && links != nullptr; ++index) {
        IDeviceRuntime* dependency = dependencyRuntimeAt(index);
        if (dependency == nullptr) {
            continue;
        }
        if (links[index].role == DeviceRole::PixelStrip) {
            targetStrip_ = const_cast<IPixelStripRuntime*>(dependency->pixelStripRuntime());
        } else if (links[index].role == DeviceRole::Condition && conditionCount_ < kMaxPixelEffectAlertConditions) {
            conditions_[conditionCount_++] = ConditionSource{dependency->statusRuntime(), links[index].invert};
        }
    }
    hasApplied_ = false;
    wasSatisfied_ = false;
}

bool PixelEffectAlertDevice::dependenciesAvailable() const {
    return dependencyRuntime(DeviceRole::PixelStrip) != nullptr && targetStrip_ != nullptr;
}

bool PixelEffectAlertDevice::dependencyBlocked() const {
    const IDeviceRuntime* stripRuntime = dependencyRuntime(DeviceRole::PixelStrip);
    return stripRuntime == nullptr || targetStrip_ == nullptr || stripRuntime->status() != DeviceStatus::Ready;
}

bool PixelEffectAlertDevice::dependencyIsDisabled() const {
    const IDeviceRuntime* stripRuntime = dependencyRuntime(DeviceRole::PixelStrip);
    return stripRuntime != nullptr && stripRuntime->status() == DeviceStatus::Disabled;
}

void PixelEffectAlertDevice::updateBlink(uint32_t now) {
    if (targetStrip_ == nullptr) {
        return;
    }
    const bool satisfied = conditionsSatisfied();
    if (!satisfied) {
        if (!hasApplied_ || wasSatisfied_ || appliedOn_) {
            targetStrip_->fill(PixelColor{});
            targetStrip_->show(now);
            appliedOn_ = false;
            hasApplied_ = true;
            lastToggleAt_ = now;
            markRuntimeStateDirty();
        }
        wasSatisfied_ = false;
        return;
    }
    if (!hasApplied_ || !wasSatisfied_) {
        // First tick ever, or the condition just became satisfied: repaint on-phase immediately
        // and restart the blink clock, rather than resuming a timer left over from the black
        // (unsatisfied) state.
        appliedOn_ = true;
        lastToggleAt_ = now;
        targetStrip_->fill(config_.color);
        targetStrip_->show(now);
        hasApplied_ = true;
        wasSatisfied_ = true;
        markRuntimeStateDirty();
        return;
    }
    const uint32_t elapsed = now - lastToggleAt_;
    if (elapsed < config_.blinkIntervalMs) {
        return;
    }
    const uint32_t intervals = elapsed / config_.blinkIntervalMs;
    lastToggleAt_ += intervals * config_.blinkIntervalMs;
    if (intervals % 2U == 0U) {
        return;
    }
    appliedOn_ = !appliedOn_;
    targetStrip_->fill(appliedOn_ ? config_.color : PixelColor{});
    targetStrip_->show(now);
    markRuntimeStateDirty();
}

SM_STATE(PixelEffectAlertDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(PixelEffectAlertDevice::Starting) {
    status_ = DeviceStatus::Starting;
    refreshCapabilityCache();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }

    clearStartRequested();
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(PixelEffectAlertDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    refreshCapabilityCache();
    clearReconfigureRequested();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(PixelEffectAlertDevice::Ready) {
    status_ = DeviceStatus::Ready;
    const uint32_t now = uptime();
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyBlocked()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (reconfigureRequested_) {
        SM_GOTO(Reconfiguring);
    }

    updateBlink(now);
}

SM_STATE(PixelEffectAlertDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (disableRequested_) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (dependencyIsDisabled()) {
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if ((reconfigureRequested_ || startRequested_) && dependenciesAvailable() && !dependencyBlocked()) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(PixelEffectAlertDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        SM_GOTO(Deleting);
    }
    if (!disableRequested_ && config_.enabled != 0U) {
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(PixelEffectAlertDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
