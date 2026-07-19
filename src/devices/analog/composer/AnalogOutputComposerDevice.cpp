#include "devices/analog/composer/AnalogOutputComposerDevice.h"

#include "devices/analog/AnalogOutputDeviceConfig.h"
#include "devices/core/ConfigCodec.h"
#include "devices/registry/DeviceRetainedDataStore.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS AnalogOutputComposerDevice

AnalogOutputComposerDevice::AnalogOutputComposerDevice(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : AnalogOutputComposerDevice([&configBlob]() {
          AnalogOutputComposerDeviceConfigV1 config{};
          (void)decodeAnalogOutputComposerDeviceConfig(configBlob.data(), configBlob.size(), config);
          return config;
      }()) {
    bindDeviceIdentity(record, configBlob);
}

AnalogOutputComposerDevice::AnalogOutputComposerDevice(const AnalogOutputComposerDeviceConfigV1& config)
    : DeviceRuntimeBase((PState)&AnalogOutputComposerDevice::Idle), config_(config) {}

const AnalogOutputComposerDeviceConfigV1& AnalogOutputComposerDevice::config() const {
    return config_;
}

const DeviceBaseConfigV1& AnalogOutputComposerDevice::baseConfig() const {
    return config_;
}

AnalogOutputMode AnalogOutputComposerDevice::analogOutputGroupMode() const {
    return mode_;
}

bool AnalogOutputComposerDevice::requestAnalogOutputGroupMode(const AnalogOutputMode mode, const uint32_t now) {
    if (status() != DeviceStatus::Ready ||
        (mode != AnalogOutputMode::Off && mode != AnalogOutputMode::Manual && mode != AnalogOutputMode::Scheduled)) {
        return false;
    }
    if (!applyModeToOutputs(mode, now)) {
        return false;
    }
    setMode(mode);
    return true;
}

bool AnalogOutputComposerDevice::handleCommand(const DeviceCommand& command) {
    if (command.type != DeviceCommandType::Custom) {
        return false;
    }
    if (command.payload.equals("off")) {
        return requestAnalogOutputGroupMode(AnalogOutputMode::Off, uptime());
    }
    if (command.payload.equals("manual")) {
        return requestAnalogOutputGroupMode(AnalogOutputMode::Manual, uptime());
    }
    if (command.payload.equals("scheduled")) {
        return requestAnalogOutputGroupMode(AnalogOutputMode::Scheduled, uptime());
    }
    return false;
}

bool AnalogOutputComposerDevice::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = analogOutputComposerDeviceConfigSize(config_);
    return encodeFixedConfigBlob(AnalogOutputComposerDeviceConfigV1::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan AnalogOutputComposerDevice::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    AnalogOutputComposerDeviceConfigV1 next{};
    (void)decodeAnalogOutputComposerDeviceConfig(configBlob.data(), configBlob.size(), next);
    return {};
}

bool AnalogOutputComposerDevice::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    AnalogOutputComposerDeviceConfigV1 next{};
    if (!decodeAnalogOutputComposerDeviceConfig(configBlob.data(), configBlob.size(), next)) {
        return false;
    }
    config_ = next;
    return true;
}

bool AnalogOutputComposerDevice::retainedStateDirty() const {
    return retainedStateDirty_;
}

void AnalogOutputComposerDevice::clearRetainedStateDirty() {
    retainedStateDirty_ = false;
}

DeviceValidationResult AnalogOutputComposerDevice::saveRetainedState(DeviceRetainedDataStore& store) const {
    AnalogOutputComposerRetainedStateV1 state{};
    state.deviceId = deviceId();
    state.mode = static_cast<uint8_t>(mode_);
    return store.save(state);
}

DeviceValidationResult AnalogOutputComposerDevice::loadRetainedState(DeviceRetainedDataStore& store) {
    AnalogOutputComposerRetainedStateV1 state{};
    const DeviceValidationResult result = store.load(deviceId(), state);
    if (!result.ok()) {
        return result;
    }
    if (state.mode <= static_cast<uint8_t>(AnalogOutputMode::Off)) {
        mode_ = static_cast<AnalogOutputMode>(state.mode);
    }
    return {};
}

DeviceTypeDescriptor AnalogOutputComposerDevice::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kAnalogOutputComposerDeviceTypeId;
    descriptor.name = "AnalogOutputComposerDevice";
    descriptor.currentConfigVersion = kAnalogOutputComposerDeviceConfigVersion;
    descriptor.supportsCommands = true;
    descriptor.supportsRetainedState = true;
    descriptor.ticks1s = true;
    descriptor.dependencyRequirements = {DeviceDependencyRequirement{DeviceRole::AnalogOutput, true}};
    descriptor.exclusiveDependencyRoles = ProvidedRoles::of({DeviceRole::AnalogOutput});
    descriptor.providedRoles = ProvidedRoles::of({IAnalogOutputGroupRuntime::kProvidedRole});
    descriptor.createRuntime = &AnalogOutputComposerDevice::createRuntime;
    descriptor.validateConfig = &AnalogOutputComposerDevice::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> AnalogOutputComposerDevice::createRuntime(const DeviceRegistryEntry& record,
                                                                          const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new AnalogOutputComposerDevice(record, configBlob));
}

DeviceValidationResult AnalogOutputComposerDevice::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    if (record.dependencyDeviceId(DeviceRole::AnalogOutput) == 0U) {
        return {DeviceError::InvalidRelationship, "analog output dependency is required"};
    }
    AnalogOutputComposerDeviceConfigV1 config{};
    if (!decodeAnalogOutputComposerDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "analog output composer config is invalid"};
    }
    return config.validate();
}

IAnalogOutputRuntime* AnalogOutputComposerDevice::outputAt(const uint8_t index) const {
    const DeviceDependencyLink* links = dependencyLinks();
    if (index >= dependencyCount() || links == nullptr || links[index].role != DeviceRole::AnalogOutput) {
        return nullptr;
    }
    IDeviceRuntime* runtime = dependencyRuntimeAt(index);
    if (runtime == nullptr) {
        return nullptr;
    }
    return const_cast<IAnalogOutputRuntime*>(runtime->analogOutputRuntime());
}

IScheduledAnalogOutputRuntime* AnalogOutputComposerDevice::scheduledOutputAt(const uint8_t index) const {
    if (index >= dependencyCount()) {
        return nullptr;
    }
    IDeviceRuntime* runtime = dependencyRuntimeAt(index);
    return runtime != nullptr ? const_cast<IScheduledAnalogOutputRuntime*>(runtime->scheduledAnalogOutputRuntime()) : nullptr;
}

bool AnalogOutputComposerDevice::applyModeToOutputs(const AnalogOutputMode mode, const uint32_t now) const {
    bool accepted = true;
    for (uint8_t index = 0U; index < dependencyCount(); ++index) {
        IScheduledAnalogOutputRuntime* scheduled = scheduledOutputAt(index);
        if (scheduled != nullptr) {
            if (scheduled->analogOutputMode() != mode || mode == AnalogOutputMode::Off) {
                accepted = scheduled->requestAnalogOutputMode(mode, now) && accepted;
            }
            continue;
        }
        IAnalogOutputRuntime* output = outputAt(index);
        if (mode == AnalogOutputMode::Off && output != nullptr && output->currentOutputState() != 0U) {
            accepted = output->requestOutputState(0U, now) && accepted;
        }
    }
    return accepted;
}

bool AnalogOutputComposerDevice::outputsReady() const {
    if (dependencyCount() == 0U) {
        return false;
    }
    for (uint8_t index = 0U; index < dependencyCount(); ++index) {
        const IDeviceRuntime* runtime = dependencyRuntimeAt(index);
        if (runtime == nullptr || runtime->status() != DeviceStatus::Ready || outputAt(index) == nullptr) {
            return false;
        }
    }
    return true;
}

SM_STATE(AnalogOutputComposerDevice::Idle) {
    status_ = DeviceStatus::Creating;
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(AnalogOutputComposerDevice::Starting) {
    status_ = DeviceStatus::Starting;
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (!outputsReady()) {
        SM_GOTO(DependencyBlocked);
    }
    clearStartRequested();
    SM_GOTO(Ready);
}

SM_STATE(AnalogOutputComposerDevice::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    clearReconfigureRequested();
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (!outputsReady()) {
        SM_GOTO(DependencyBlocked);
    }
    SM_GOTO(Ready);
}

SM_STATE(AnalogOutputComposerDevice::Ready) {
    status_ = DeviceStatus::Ready;
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        SM_GOTO(Reconfiguring);
    }
    if (!outputsReady()) {
        SM_GOTO(DependencyBlocked);
    }
    (void)applyModeToOutputs(mode_, uptime());
}

SM_STATE(AnalogOutputComposerDevice::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        SM_GOTO(Disabled);
    }
    if (outputsReady()) {
        clearStartRequested();
        clearReconfigureRequested();
        SM_GOTO(Ready);
    }
}

SM_STATE(AnalogOutputComposerDevice::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        SM_GOTO(Deleting);
    }
    if (!disableRequested_ && config_.enabled != 0U) {
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(AnalogOutputComposerDevice::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

void AnalogOutputComposerDevice::setMode(const AnalogOutputMode mode) {
    if (mode_ == mode) {
        return;
    }
    mode_ = mode;
    retainedStateDirty_ = true;
    markRuntimeStateDirty();
}

} // namespace ewfm
