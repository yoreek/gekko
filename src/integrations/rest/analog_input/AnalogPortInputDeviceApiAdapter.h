#pragma once

#include "devices/analog/input/port/AnalogPortInputDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class AnalogPortInputDeviceApiAdapter final
    : public TypedDeviceApiAdapter<AnalogPortInputDeviceApiAdapter, AnalogPortInputDevice, AnalogPortInputDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "analog_port_input";

    void writeRuntimeJson(const AnalogPortInputDevice& device, JsonObject runtimeJson) const;
};

} // namespace ewfm
