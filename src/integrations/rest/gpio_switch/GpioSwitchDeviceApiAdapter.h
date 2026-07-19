#pragma once

#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class GpioSwitchDeviceApiAdapter final
    : public TypedDeviceApiAdapter<GpioSwitchDeviceApiAdapter, GpioSwitchDevice, GpioSwitchDeviceConfigV3> {
public:
    static constexpr const char* kTypeName = "gpio_switch";

    void writeRuntimeJson(const GpioSwitchDevice& device, JsonObject runtimeJson) const {
        JsonObject outputJson = runtimeJson.createNestedObject("output");
        outputJson["state"] = device.currentOutputState();
        outputJson["physicalLevel"] = device.physicalOutputState();
    }
};

} // namespace ewfm
