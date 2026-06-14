#include "devices/switch/SwitchDeviceBase.h"

namespace ewfm {

#undef SM_CLASS
#define SM_CLASS SwitchDeviceBase

namespace {
constexpr uint8_t kSwitchRetainedStateVersion = 1;
constexpr size_t kSwitchRetainedStatePayloadSize = 2;
static_assert(kSwitchRetainedStatePayloadSize <= kMaxRetainedStateBytes, "switch retained state payload exceeds retained-state bound");
} // namespace

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
    return config_.restorePreviousState != 0U;
}

bool SwitchDeviceBase::retainedStateDirty() const {
    return retainedStateDirty_;
}

bool SwitchDeviceBase::serializeRetainedState(RetainedStateRecord& record) const {
    if (!restorePreviousState()) {
        return false;
    }
    record.payload.resize(kSwitchRetainedStatePayloadSize);
    record.payload[0] = static_cast<char>(kSwitchRetainedStateVersion);
    record.payload[1] = static_cast<char>(static_cast<uint8_t>(outputState_));
    return true;
}

bool SwitchDeviceBase::applyRetainedStateRecord(const RetainedStateRecord& record) {
    if (record.payload.size() != kSwitchRetainedStatePayloadSize) {
        return false;
    }
    if (static_cast<uint8_t>(record.payload[0]) != kSwitchRetainedStateVersion) {
        return false;
    }

    OutputState state{};
    if (!outputStateFromByte(static_cast<uint8_t>(record.payload[1]), state)) {
        return false;
    }
    applyRetainedState(state);
    return true;
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
    return config_.inverted != 0U;
}

bool SwitchDeviceBase::applyConfiguredOutput(OutputState state, uint32_t now, bool markRetainedDirty) {
    if (!supportsOutputState(state)) {
        return false;
    }

    const bool physicalLevel = state == OutputState::On ? !inverted() : inverted();
    DeviceValidationResult result = applyHardwareOutput(state, physicalLevel, now);
    if (!result.ok()) {
        status_ = DeviceStatus::Faulted;
        return false;
    }

    outputState_ = state;
    if (state != OutputState::Disabled) {
        physicalOutputState_ = physicalLevel;
    }
    if (markRetainedDirty) {
        retainedStateDirty_ = true;
    }
    return true;
}

bool SwitchDeviceBase::parseSetStateCommand(const DeviceCommand& command, OutputState& state) const {
    if (command.type != DeviceCommandType::Custom) {
        return false;
    }
    if (command.payload.equals("state=on")) {
        state = OutputState::On;
        return true;
    }
    if (command.payload.equals("state=off")) {
        state = OutputState::Off;
        return true;
    }
    if (command.payload.equals("state=disabled")) {
        state = OutputState::Disabled;
        return true;
    }
    return false;
}

bool SwitchDeviceBase::shouldSaveRetainedState() const {
    return restorePreviousState();
}

SM_STATE(SwitchDeviceBase::Idle) {
    status_ = DeviceStatus::Creating;
    if (startRequested_) {
        SM_GOTO(Starting);
    }
}

SM_STATE(SwitchDeviceBase::Starting) {
    status_ = DeviceStatus::Starting;
    if (!parentReady()) {
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
    if (!parentReady()) {
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
        if (parentReady()) {
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
