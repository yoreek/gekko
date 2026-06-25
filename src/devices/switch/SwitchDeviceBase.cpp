#include "devices/switch/SwitchDeviceBase.h"

#include "devices/registry/DeviceRetainedDataStore.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS SwitchDeviceBase

SwitchDeviceBase::SwitchDeviceBase(const SwitchDeviceConfigV1& config)
    : DeviceRuntimeBase((PState)&SwitchDeviceBase::Idle), config_(config) {
    OutputState startup{};
    if (switchConfigStartupState(config_, startup)) {
        outputState_ = startup;
    }
}

OutputState SwitchDeviceBase::outputState() const {
    return outputState_;
}

bool SwitchDeviceBase::physicalOutputState() const {
    return physicalOutputState_;
}

bool SwitchDeviceBase::restorePreviousState() const {
    return config_.restorePreviousState;
}

bool SwitchDeviceBase::enabled() const {
    return config_.enabled != 0U;
}

const char* SwitchDeviceBase::name() const {
    return config_.name;
}

bool SwitchDeviceBase::retainedStateDirty() const {
    return retainedStateDirty_;
}

const ISwitchOutputRuntime* SwitchDeviceBase::switchOutputRuntime() const {
    return this;
}

OutputStateMask SwitchDeviceBase::supportedOutputStateMask() const {
    return supportedOutputStateMaskImpl();
}

OutputState SwitchDeviceBase::currentOutputState() const {
    return outputState_;
}

bool SwitchDeviceBase::requestOutputState(OutputState state, uint32_t now) {
    return applyConfiguredOutput(state, now, shouldSaveRetainedState());
}

void SwitchDeviceBase::bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) {
    DeviceRuntimeBase::bindDeviceIdentity(record, config);
}

DeviceConfigUpdatePlan SwitchDeviceBase::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    SwitchDeviceConfigV1 config{};
    if (!decodeSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool invertedChanged = config.inverted != switchConfig().inverted;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = invertedChanged;
    plan.resetStateMachine = invertedChanged;
    return plan;
}

bool SwitchDeviceBase::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    SwitchDeviceConfigV1 config{};
    if (!decodeSwitchDeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    setSwitchConfig(config);
    return true;
}

DeviceValidationResult SwitchDeviceBase::saveRetainedState(DeviceRetainedDataStore& store) const {
    if (!restorePreviousState()) {
        return {};
    }
    SwitchRetainedStateRecord record{};
    record.recordVersion = kRetainedStateRecordVersion;
    record.deviceId = deviceId();
    record.outputState = outputState_;
    return store.save(record);
}

DeviceValidationResult SwitchDeviceBase::loadRetainedState(DeviceRetainedDataStore& store) {
    SwitchRetainedStateRecord record{};
    const DeviceValidationResult result = store.load(deviceId(), record);
    if (!result.ok()) {
        return result;
    }
    applyRetainedState(record.outputState);
    return {};
}

void SwitchDeviceBase::clearRetainedStateDirty() {
    retainedStateDirty_ = false;
}

void SwitchDeviceBase::applyRetainedState(OutputState state) {
    if (!supportsOutputState(state)) {
        return;
    }
    retainedOutputState_ = state;
    retainedStateAvailable_ = true;
}

bool SwitchDeviceBase::handleCommand(const DeviceCommand& command) {
    if (command.type == DeviceCommandType::SetStatus) {
        if (command.payload.equals("fault")) {
            requestFault();
            return true;
        }
        if (command.payload.equals("ready")) {
            clearFaultRequested();
            clearReconfigureRequested();
            clearDisableRequested();
            return true;
        }
    }

    OutputState requested{};
    if (!parseSetStateCommand(command, requested)) {
        return false;
    }
    return setOutputState(requested, uptime());
}

bool SwitchDeviceBase::setOutputState(OutputState state, uint32_t now) {
    if (status_ != DeviceStatus::Ready) {
        return false;
    }
    return applyConfiguredOutput(state, now, shouldSaveRetainedState());
}

bool SwitchDeviceBase::supportsOutputState(OutputState state) const {
    return outputStateIsSupported(state, supportedOutputStateMask());
}

OutputState SwitchDeviceBase::startupState() const {
    OutputState state{OutputState::Off};
    (void)switchConfigStartupState(config_, state);
    return state;
}

OutputState SwitchDeviceBase::safeState() const {
    OutputState state{OutputState::Off};
    (void)switchConfigSafeState(config_, state);
    return state;
}

bool SwitchDeviceBase::inverted() const {
    return config_.inverted;
}

const SwitchDeviceConfigV1& SwitchDeviceBase::switchConfig() const {
    return config_;
}

void SwitchDeviceBase::writeDeviceJson(JsonObject output) const {
    writeCommonDeviceJson(output);
    config_.writeJson(output);
    JsonObject outputObject = output.createNestedObject("output");
    outputObject["state"] = outputStateName(outputState_);
    outputObject["physical_level"] = physicalOutputState_;
}

bool SwitchDeviceBase::applyConfiguredOutput(OutputState state, uint32_t now, bool markRetainedDirty) {
    if (!supportsOutputState(state)) {
        return false;
    }

    const bool physicalLevel = state == OutputState::On ? !inverted() : inverted();
    const bool outputChanged = state != outputState_ || (state != OutputState::Disabled && physicalLevel != physicalOutputState_);
    if (status_ == DeviceStatus::Ready && !outputChanged) {
        return true;
    }
    DeviceValidationResult result = applyHardwareOutput(state, physicalLevel, now);
    if (!result.ok()) {
        status_ = DeviceStatus::Faulted;
        return false;
    }

    outputState_ = state;
    if (state != OutputState::Disabled) {
        physicalOutputState_ = physicalLevel;
    }
    if (outputChanged) {
        markRuntimeStateDirty();
    }
    if (markRetainedDirty) {
        retainedStateDirty_ = true;
    }
    return true;
}

bool SwitchDeviceBase::parseSetStateCommand(const DeviceCommand& command, OutputState& state) const {
    if (command.type != DeviceCommandType::SetOutput && command.type != DeviceCommandType::Custom) {
        return false;
    }
    if (command.type == DeviceCommandType::Custom && command.payload.equals("state=on")) {
        state = OutputState::On;
        return true;
    }
    if (command.type == DeviceCommandType::Custom && command.payload.equals("state=off")) {
        state = OutputState::Off;
        return true;
    }
    if (command.type == DeviceCommandType::Custom && command.payload.equals("state=disabled")) {
        state = OutputState::Disabled;
        return true;
    }
    if (command.payload.equals("on")) {
        state = OutputState::On;
        return true;
    }
    if (command.payload.equals("off")) {
        state = OutputState::Off;
        return true;
    }
    if (command.payload.equals("disabled")) {
        state = OutputState::Disabled;
        return true;
    }
    return false;
}

bool SwitchDeviceBase::shouldSaveRetainedState() const {
    return restorePreviousState();
}

void SwitchDeviceBase::setSwitchConfig(const SwitchDeviceConfigV1& config) {
    config_ = config;
}

SM_STATE(SwitchDeviceBase::Idle) {
    status_ = DeviceStatus::Creating;
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(SwitchDeviceBase::Starting) {
    status_ = DeviceStatus::Starting;
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        SM_GOTO(Deleting);
    }
    if (!configureHardware(uptime()).ok()) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        (void)applyConfiguredOutput(safeState(), uptime(), false);
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }

    const OutputState selectedState = restorePreviousState() && retainedStateAvailable_ ? retainedOutputState_ : startupState();
    if (!applyConfiguredOutput(selectedState, uptime(), false)) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }

    startRequested_ = false;
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(SwitchDeviceBase::Ready) {
    status_ = DeviceStatus::Ready;
    if (!dependenciesReady()) {
        status_ = DeviceStatus::DependencyBlocked;
        SM_GOTO(DependencyBlocked);
    }
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        SM_GOTO(Deleting);
    }
    if (disableRequested_ || config_.enabled == 0U) {
        (void)applyConfiguredOutput(safeState(), uptime(), false);
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
    if (faultRequested_) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
}

SM_STATE(SwitchDeviceBase::Reconfiguring) {
    status_ = DeviceStatus::Reconfiguring;
    reconfigureRequested_ = false;
    if (!configureHardware(uptime()).ok() || !applyConfiguredOutput(outputState_, uptime(), false)) {
        status_ = DeviceStatus::Faulted;
        SM_GOTO(Faulted);
    }
    status_ = DeviceStatus::Ready;
    SM_GOTO(Ready);
}

SM_STATE(SwitchDeviceBase::DependencyBlocked) {
    status_ = DeviceStatus::DependencyBlocked;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        SM_GOTO(Deleting);
    }
    if (disableRequested_) {
        (void)applyConfiguredOutput(safeState(), uptime(), false);
        status_ = DeviceStatus::Disabled;
        SM_GOTO(Disabled);
    }
    if (reconfigureRequested_ || startRequested_) {
        if (dependenciesReady()) {
            status_ = DeviceStatus::Reconfiguring;
            SM_GOTO(Reconfiguring);
        }
    }
}

SM_STATE(SwitchDeviceBase::Disabled) {
    status_ = DeviceStatus::Disabled;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(SwitchDeviceBase::Faulted) {
    status_ = DeviceStatus::Faulted;
    if (deleteRequested_) {
        status_ = DeviceStatus::Deleting;
        setDeleted();
        releaseHardware(uptime());
        SM_GOTO(Deleting);
    }
    if (reconfigureRequested_) {
        faultRequested_ = false;
        status_ = DeviceStatus::Reconfiguring;
        SM_GOTO(Reconfiguring);
    }
}

SM_STATE(SwitchDeviceBase::Deleting) {
    status_ = DeviceStatus::Deleting;
    setDeleted();
}

} // namespace ewfm
