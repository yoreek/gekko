#pragma once

#include "devices/core/DeviceRuntimeBase.h"

namespace ewfm {

class AnalogOutputDecoratorDeviceBase : public DeviceRuntimeBase, public IAnalogOutputRuntime {
public:
    const IAnalogOutputRuntime* analogOutputRuntime() const override {
        return this;
    }

    void setDependencyRuntime(DeviceRole role, IDeviceRuntime* dependencyRuntime) override;
    void setDependencyRuntimeAt(uint8_t index, IDeviceRuntime* dependencyRuntime) override;
    void bindDeviceIdentity(const DeviceRegistryEntry& record, const DeviceConfigBlob& config) override;
    uint16_t currentOutputState() const override;
    bool handleCommand(const DeviceCommand& command) override;

protected:
    AnalogOutputDecoratorDeviceBase();

    IAnalogOutputRuntime* targetOutput() const;
    bool forwardOutputState(uint16_t state, uint32_t now);
    virtual void onReadyTick(uint32_t now) = 0;
    virtual void onTargetAttached(uint32_t now);

private:
    State Idle();
    State Starting();
    State Reconfiguring();
    State Ready();
    State DependencyBlocked();
    State Disabled();
    State Deleting();

    void refreshTarget();
    bool targetReady() const;
    bool parseOutputCommand(const DeviceCommand& command, uint16_t& state) const;

    IAnalogOutputRuntime* targetOutput_{nullptr};
};

} // namespace ewfm
