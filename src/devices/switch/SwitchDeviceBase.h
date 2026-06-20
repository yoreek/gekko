#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/switch/SwitchDeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class SwitchDeviceBase : public DeviceRuntimeBase {
public:
    OutputState outputState() const;
    bool physicalOutputState() const;
    bool restorePreviousState() const;
    bool retainedStateDirty() const override;
    bool serializeRetainedState(RetainedStateRecord& record) const override;
    bool applyRetainedStateRecord(const RetainedStateRecord& record) override;
    void clearRetainedStateDirty() override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    void applyRetainedState(OutputState state);
    void writeDeviceJson(JsonObject output) const;
    bool handleCommand(const DeviceCommand& command) override;

protected:
    explicit SwitchDeviceBase(const SwitchDeviceConfigV1& config);

    bool setOutputState(OutputState state, uint32_t now);
    bool supportsOutputState(OutputState state) const;
    OutputState startupState() const;
    OutputState safeState() const;
    bool inverted() const;
    const SwitchDeviceConfigV1& switchConfig() const;

    virtual DeviceValidationResult configureHardware(uint32_t now) = 0;
    virtual DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) = 0;
    virtual void releaseHardware(uint32_t now) = 0;
    virtual OutputStateMask supportedOutputStateMask() const = 0;

private:
    State Idle();
    State Starting();
    State Ready();
    State Reconfiguring();
    State DependencyBlocked();
    State Disabled();
    State Faulted();
    State Deleting();

    bool applyConfiguredOutput(OutputState state, uint32_t now, bool markRetainedDirty);
    bool parseSetStateCommand(const DeviceCommand& command, OutputState& state) const;
    bool shouldSaveRetainedState() const;

    SwitchDeviceConfigV1 config_{};
    OutputState outputState_{OutputState::Off};
    OutputState retainedOutputState_{OutputState::Off};
    bool physicalOutputState_{false};
    bool retainedStateAvailable_{false};
    bool retainedStateDirty_{false};
};

} // namespace ewfm
