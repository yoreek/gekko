#pragma once

#include "devices/analog/scheduled/ScheduledAnalogOutputDevice.h"
#include "integrations/rest/analog_output/TypedAnalogOutputDependentApiAdapter.h"

namespace ewfm {

class ScheduledAnalogOutputDeviceApiAdapter final
    : public TypedAnalogOutputDependentApiAdapter<ScheduledAnalogOutputDeviceApiAdapter, ScheduledAnalogOutputDevice,
                                                  ScheduledAnalogOutputDeviceConfigV2, 1U> {
public:
    static constexpr const char* kTypeName = "scheduled_analog_output";

    void writeRuntimeJson(const ScheduledAnalogOutputDevice& device, JsonObject runtimeJson) const {
        JsonObject output = runtimeJson.createNestedObject("output");
        output["state"] = analogOutputStateToPercent(device.currentOutputState());
        output["requestedState"] = analogOutputStateToPercent(device.requestedAnalogOutputState());
        output["mode"] = analogOutputModeName(device.analogOutputMode());
        output["timeValid"] = device.analogOutputTimeValid();
    }
};

} // namespace ewfm
