#include "devices/switch/SwitchDeviceBase.h"

#include "devices/switch/SwitchOutputState.h"

namespace ewfm {

SwitchDeviceBase::SwitchDeviceBase(const SwitchDeviceConfigV2& config)
    : AbstractOutputDevice<bool, ISwitchOutputRuntime>(config.startupState) {}

bool SwitchDeviceBase::physicalOutputState() const {
    return physicalOutputState_;
}

const ISwitchOutputRuntime* SwitchDeviceBase::switchOutputRuntime() const {
    return this;
}

DeviceConfigUpdatePlan SwitchDeviceBase::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    SwitchDeviceConfigV2 config{};
    if (!decodeValidatedFixedConfigBlob(SwitchDeviceConfigV2::kMagic, configBlob.data(), configBlob.size(), config)) {
        return {};
    }

    const bool invertedChanged = config.inverted != this->config().inverted;

    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = invertedChanged;
    plan.resetStateMachine = invertedChanged;
    return plan;
}

bool SwitchDeviceBase::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    SwitchDeviceConfigV2 config{};
    if (!decodeValidatedFixedConfigBlob(SwitchDeviceConfigV2::kMagic, configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    mutableConfig() = config;
    return true;
}

void SwitchDeviceBase::applyRetainedState(const StateType state) {
    (void)AbstractOutputDevice<bool, ISwitchOutputRuntime>::applyRetainedState(state);
}

bool SwitchDeviceBase::invertedState(const bool state) const {
    return !state;
}

bool SwitchDeviceBase::stateIsValid(const StateType state) const {
    (void)state;
    return true;
}

void SwitchDeviceBase::hardwareOutputApplied(const bool state) {
    physicalOutputState_ = state;
}

bool SwitchDeviceBase::parseOutputCommand(const DeviceCommand& command, StateType& state) const {
    return parseSwitchOutputStateCommand(command, state);
}

} // namespace ewfm
