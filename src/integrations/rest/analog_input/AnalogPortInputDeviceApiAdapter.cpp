#include "integrations/rest/analog_input/AnalogPortInputDeviceApiAdapter.h"

#include "devices/analog/input/AnalogInputJson.h"

namespace ewfm {

void AnalogPortInputDeviceApiAdapter::writeRuntimeJson(const AnalogPortInputDevice& device, JsonObject runtimeJson) const {
    JsonObject outputJson = runtimeJson.createNestedObject("output");
    JsonObject analogInput = outputJson.createNestedObject("analogInput");
    writeAnalogInputOutputJson(device.reading(), device.outputStatus(), analogInput);
}

} // namespace ewfm
