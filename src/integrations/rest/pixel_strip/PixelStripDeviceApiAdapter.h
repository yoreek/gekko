#pragma once

#include "devices/pixel/PixelStripDevice.h"
#include "devices/pixel/PixelStripDeviceConfig.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class PixelStripDeviceApiAdapter final
    : public TypedDeviceApiAdapter<PixelStripDeviceApiAdapter, PixelStripDevice, PixelStripDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "pixel_strip";

    void writeRuntimeJson(const PixelStripDevice& device, JsonObject runtimeJson) const {
        JsonObject output = runtimeJson.createNestedObject("output");
        output["pixelCount"] = device.pixelCount();
        output["brightness"] = pixelBrightnessToPercent(device.liveBrightness());
        output["on"] = device.liveOn();
    }
};

} // namespace ewfm
