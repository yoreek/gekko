#include "devices/pixel/effects/PixelEffectSolidDevice.h"

#include "devices/core/ConfigCodec.h"
#include "devices/registry/DeviceRetainedDataStore.h"

#include <ArduinoJson.h>
#include <string_view>

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS PixelEffectSolidDevice

PixelEffectSolidDevice::PixelEffectSolidDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : PixelEffectSolidDevice([&configBlob]() {
          PixelEffectSolidDeviceConfigV1 config{};
          (void)decodePixelEffectSolidDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

PixelEffectSolidDevice::PixelEffectSolidDevice(const PixelEffectSolidDeviceConfigV1& config)
    : DeviceRuntimeBase((PState)&PixelEffectSolidDevice::Idle), config_(config) {}

const PixelEffectSolidDeviceConfigV1& PixelEffectSolidDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& PixelEffectSolidDevice::baseConfig() const {
    return config_;
}

void PixelEffectSolidDevice::setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntime(role, dependencyRuntime);
    refreshCapabilityCache();
}

void PixelEffectSolidDevice::setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) {
    DeviceRuntimeBase::setDependencyRuntimeAt(index, dependencyRuntime);
    refreshCapabilityCache();
}

void PixelEffectSolidDevice::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
    refreshCapabilityCache();
}

bool PixelEffectSolidDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = pixelEffectSolidDeviceConfigSize(config_);
    return encodeFixedConfigBlob(PixelEffectSolidDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan PixelEffectSolidDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    PixelEffectSolidDeviceConfigV1 next{};
    (void)decodePixelEffectSolidDeviceConfig(configBlob.data(), configBlob.size(), next);
    return {};
}

bool PixelEffectSolidDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    PixelEffectSolidDeviceConfigV1 next{};
    if (!decodePixelEffectSolidDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return false;
    }
    config_ = next;
    return true;
}

DeviceTypeDescriptor PixelEffectSolidDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kPixelEffectSolidDeviceTypeId;
    descriptor.name = "PixelEffectSolidDevice";
    descriptor.currentConfigVersion = kPixelEffectSolidDeviceConfigVersion;
    descriptor.maxDependents = 0;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::PixelStrip, true}};
    descriptor.exclusiveDependencyRoles = ProvidedRoles::of({DeviceRole::PixelStrip});
    descriptor.createRuntime = &PixelEffectSolidDevice::createRuntime;
    descriptor.validateConfig = &PixelEffectSolidDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> PixelEffectSolidDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                      const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new PixelEffectSolidDevice(record, configBlob));
}

DeviceValidationResult PixelEffectSolidDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::PixelStrip) == 0U) {
        return {DeviceError::InvalidRelationship, "pixel strip dependency is required"};
    }
    PixelEffectSolidDeviceConfigV1 config{};
    if (!decodePixelEffectSolidDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "pixel effect solid config is invalid"};
    }
    return config.validate();
}

void PixelEffectSolidDevice::refreshCapabilityCache() {
    IDeviceRuntime* dependency = dependencyRuntime(DeviceRole::PixelStrip);
    targetStrip_ = dependency != nullptr ? const_cast<IPixelStripRuntime*>(dependency->pixelStripRuntime()) : nullptr;
    liveColor_ = (config_.restorePreviousState && retainedColorAvailable_) ? retainedColor_ : config_.startupColor;
    liveOn_ = (config_.restorePreviousState && retainedOnAvailable_) ? retainedOn_ : true;
    colorApplied_ = false;
}

PixelColor PixelEffectSolidDevice::liveColor() const {
    return liveColor_;
}

bool PixelEffectSolidDevice::liveOn() const {
    return liveOn_;
}

bool PixelEffectSolidDevice::parseSetOutputColor(const DeviceCommand& command, PixelColor& color) const {
    if (command.type != DeviceCommandType::SetOutput) {
        return false;
    }
    StaticJsonDocument<192> doc;
    const std::string_view payload = command.payload.view();
    if (deserializeJson(doc, payload.data(), payload.size())) {
        return false;
    }
    if (doc["r"].isNull() || doc["g"].isNull() || doc["b"].isNull()) {
        return false;
    }
    const long r = doc["r"].as<long>();
    const long g = doc["g"].as<long>();
    const long b = doc["b"].as<long>();
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        return false;
    }
    color = PixelColor{static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)};
    return true;
}

bool PixelEffectSolidDevice::parseSetOutputOn(const DeviceCommand& command, bool& on) const {
    if (command.type != DeviceCommandType::SetOutput) {
        return false;
    }
    StaticJsonDocument<128> doc;
    const std::string_view payload = command.payload.view();
    if (deserializeJson(doc, payload.data(), payload.size())) {
        return false;
    }
    const JsonVariantConst value = doc["on"];
    if (!value.is<bool>()) {
        return false;
    }
    on = value.as<bool>();
    return true;
}

bool PixelEffectSolidDevice::handleCommand(const DeviceCommand& command) {
    if (status_ != DeviceStatus::Ready) {
        return false;
    }
    PixelColor color{};
    if (parseSetOutputColor(command, color)) {
        liveColor_ = color;
        colorApplied_ = false;
        applyColorIfNeeded(uptime());
        retainedColor_ = liveColor_;
        retainedColorAvailable_ = true;
        retainedStateDirty_ = true;
        return true;
    }
    bool on = false;
    if (parseSetOutputOn(command, on)) {
        liveOn_ = on;
        colorApplied_ = false;
        applyColorIfNeeded(uptime());
        retainedOn_ = liveOn_;
        retainedOnAvailable_ = true;
        retainedStateDirty_ = true;
        return true;
    }
    return false;
}

bool PixelEffectSolidDevice::retainedStateDirty() const {
    return retainedStateDirty_;
}

void PixelEffectSolidDevice::clearRetainedStateDirty() {
    retainedStateDirty_ = false;
}

DeviceValidationResult PixelEffectSolidDevice::saveRetainedState(DeviceRetainedDataStore& store) const {
    if (!config_.restorePreviousState) {
        return {};
    }
    PixelEffectSolidRetainedStateV1 record{};
    record.deviceId = deviceId();
    record.color = retainedColor_;
    record.on = retainedOn_;
    return store.save(record);
}

DeviceValidationResult PixelEffectSolidDevice::loadRetainedState(DeviceRetainedDataStore& store) {
    PixelEffectSolidRetainedStateV1 record{};
    const DeviceValidationResult result = store.load(deviceId(), record);
    if (!result.ok()) {
        return result;
    }
    retainedColor_ = record.color;
    retainedColorAvailable_ = true;
    retainedOn_ = record.on;
    retainedOnAvailable_ = true;
    return {};
}

bool PixelEffectSolidDevice::dependenciesAvailable() const {
    return dependencyRuntime(DeviceRole::PixelStrip) != nullptr && targetStrip_ != nullptr;
}

bool PixelEffectSolidDevice::dependencyBlocked() const {
    const IDeviceRuntime* stripRuntime = dependencyRuntime(DeviceRole::PixelStrip);
    return stripRuntime == nullptr || targetStrip_ == nullptr || stripRuntime->status() != DeviceStatus::Ready;
}

bool PixelEffectSolidDevice::dependencyIsDisabled() const {
    const IDeviceRuntime* stripRuntime = dependencyRuntime(DeviceRole::PixelStrip);
    return stripRuntime != nullptr && stripRuntime->status() == DeviceStatus::Disabled;
}

void PixelEffectSolidDevice::applyColorIfNeeded(uint32_t now) {
    if (targetStrip_ == nullptr) {
        return;
    }
    // liveOn_ gates the applied color independently of liveColor_: off always fills black
    // regardless of the configured color, on shows the actual liveColor_.
    const PixelColor effective = liveOn_ ? liveColor_ : PixelColor{};
    if (colorApplied_ && lastAppliedColor_ == effective) {
        return;
    }
    targetStrip_->fill(effective);
    targetStrip_->show(now);
    lastAppliedColor_ = effective;
    colorApplied_ = true;
    markRuntimeStateDirty();
}

SM_STATE(PixelEffectSolidDevice::Idle) {
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

SM_STATE(PixelEffectSolidDevice::Starting) {
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

SM_STATE(PixelEffectSolidDevice::Reconfiguring) {
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

SM_STATE(PixelEffectSolidDevice::Ready) {
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

    applyColorIfNeeded(now);
}

SM_STATE(PixelEffectSolidDevice::DependencyBlocked) {
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

SM_STATE(PixelEffectSolidDevice::Disabled) {
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

SM_STATE(PixelEffectSolidDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
