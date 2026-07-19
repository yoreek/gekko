#pragma once

#include "devices/analog/ledc/LedcAnalogOutputDevice.h"
#include "devices/analog/ledc/LedcAnalogOutputDeviceConfig.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class LedcAnalogOutputDeviceApiAdapter final
    : public TypedDeviceApiAdapter<LedcAnalogOutputDeviceApiAdapter, LedcAnalogOutputDevice, LedcAnalogOutputDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "analog_output";

    void writeRuntimeJson(const LedcAnalogOutputDevice& device, JsonObject runtimeJson) const {
        const IAnalogOutputRuntime::StateType state = device.currentOutputState();
        JsonObject outputJson = runtimeJson.createNestedObject("output");
        outputJson["state"] = analogOutputStateToPercent(state);
    }
};

} // namespace ewfm
