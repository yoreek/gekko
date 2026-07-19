#pragma once

#include "devices/output/AbstractOutputDevice.h"
#include "devices/switch/SwitchDeviceConfig.h"

#include <ArduinoJson.h>

namespace ewfm {

class SwitchDeviceBase : public AbstractOutputDevice<bool, ISwitchOutputRuntime>, public IStatusRuntime {
public:
    using StateType = AbstractOutputDevice<bool, ISwitchOutputRuntime>::StateType;

    bool physicalOutputState() const;
    const ISwitchOutputRuntime* switchOutputRuntime() const override;
    const IStatusRuntime* statusRuntime() const override {
        return this;
    }
    // IStatusRuntime: a condition-list consumer (e.g. AutoSwitchDevice) reads this switch's current
    // output as a boolean, uniformly with any other Condition-role dependency source.
    bool isActive() const override {
        return currentOutputState();
    }
    void applyRetainedState(StateType state);
    DeviceConfigUpdatePlan planConfigUpdate(const DeviceConfigBlob& configBlob) const override;
    bool applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) override;

protected:
    explicit SwitchDeviceBase(const SwitchDeviceConfigV2& config);
    virtual const SwitchDeviceConfigV2& config() const override = 0;
    virtual SwitchDeviceConfigV2& mutableConfig() = 0;

private:
    bool invertedState(bool state) const override;
    bool parseOutputCommand(const DeviceCommand& command, StateType& state) const override;
    bool stateIsValid(StateType state) const override;
    void hardwareOutputApplied(bool state) override;

    bool physicalOutputState_{false};
};

} // namespace ewfm
