#pragma once

#include "devices/analog/composer/AnalogOutputComposerDevice.h"
#include "integrations/rest/analog_output/TypedAnalogOutputDependentApiAdapter.h"

namespace ewfm {

class AnalogOutputComposerDeviceApiAdapter final
    : public TypedAnalogOutputDependentApiAdapter<AnalogOutputComposerDeviceApiAdapter, AnalogOutputComposerDevice,
                                                  AnalogOutputComposerDeviceConfigV1, kMaxDeviceDependencies> {
public:
    static constexpr const char* kTypeName = "analog_output_composer";

    void writeRuntimeJson(const AnalogOutputComposerDevice& device, JsonObject runtimeJson) const {
        JsonObject output = runtimeJson.createNestedObject("output");
        output["mode"] = analogOutputModeName(device.analogOutputGroupMode());
    }
};

} // namespace ewfm
