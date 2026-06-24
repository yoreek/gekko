#pragma once

#include "devices/core/DeviceRuntimeBase.h"
#include "devices/switch/SwitchDeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class SwitchDeviceBase : public DeviceRuntimeBase, public ISwitchOutputRuntime {
public:
    OutputState outputState() const;
    bool physicalOutputState() const;
    bool restorePreviousState() const;
    bool enabled() const override;
    const char* name() const override;
    bool retainedStateDirty() const override;
    const ISwitchOutputRuntime* switchOutputRuntime() const override;
    OutputStateMask supportedOutputStateMask() const override;
    OutputState currentOutputState() const override;
    bool requestOutputState(OutputState state, uint32_t now) override;
    DeviceValidationResult saveRetainedState(RetainedStateStore& store) const override;
    DeviceValidationResult loadRetainedState(RetainedStateStore& store) override;
    void clearRetainedStateDirty() override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    void applyRetainedState(OutputState state);
    void writeDeviceJson(JsonObject output) const;
    bool handleCommand(const DeviceCommand& command) override;
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

protected:
    explicit SwitchDeviceBase(const SwitchDeviceConfigV1& config);

    bool setOutputState(OutputState state, uint32_t now);
    bool supportsOutputState(OutputState state) const;
    OutputState startupState() const;
    OutputState safeState() const;
    bool inverted() const;
    const SwitchDeviceConfigV1& switchConfig() const;
    void setSwitchConfig(const SwitchDeviceConfigV1& config);

    virtual DeviceValidationResult configureHardware(uint32_t now) = 0;
    virtual DeviceValidationResult applyHardwareOutput(OutputState state, bool physicalLevel, uint32_t now) = 0;
    virtual void releaseHardware(uint32_t now) = 0;
    virtual OutputStateMask supportedOutputStateMaskImpl() const = 0;

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
