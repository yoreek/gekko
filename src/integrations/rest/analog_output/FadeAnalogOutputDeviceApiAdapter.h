#pragma once

#include "devices/analog/fade/FadeAnalogOutputDevice.h"
#include "integrations/rest/analog_output/TypedAnalogOutputDependentApiAdapter.h"

namespace ewfm {

class FadeAnalogOutputDeviceApiAdapter final
    : public TypedAnalogOutputDependentApiAdapter<FadeAnalogOutputDeviceApiAdapter, FadeAnalogOutputDevice, FadeAnalogOutputDeviceConfigV1,
                                                  1U> {
public:
    static constexpr const char* kTypeName = "fade_analog_output";

    void writeRuntimeJson(const FadeAnalogOutputDevice& device, JsonObject runtimeJson) const {
        JsonObject output = runtimeJson.createNestedObject("output");
        output["state"] = analogOutputStateToPercent(device.currentOutputState());
        output["targetState"] = analogOutputStateToPercent(device.targetOutputState());
        output["transitioning"] = device.transitioning();
    }
};

} // namespace ewfm
