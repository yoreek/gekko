#pragma once

#include "devices/analog/AnalogOutputDeviceConfig.h"
#include "devices/output/AbstractOutputDevice.h"

namespace ewfm {

class AnalogOutputDeviceBase : public AbstractOutputDevice<uint16_t, IAnalogOutputRuntime> {
public:
    const IAnalogOutputRuntime* analogOutputRuntime() const override {
        return this;
    }

protected:
    explicit AnalogOutputDeviceBase(const AnalogOutputDeviceConfigV1& config);

    virtual const AnalogOutputDeviceConfigV1& config() const override = 0;

private:
    using StateType = AbstractOutputDevice<uint16_t, IAnalogOutputRuntime>::StateType;

    uint16_t invertedState(uint16_t state) const override;
    bool parseOutputCommand(const DeviceCommand& command, StateType& state) const override;
    bool stateIsValid(StateType state) const override;
};

} // namespace ewfm
