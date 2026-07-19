#pragma once

#include "devices/sensors/binary/BinarySensorDevice.h"
#include "integrations/rest/common/TypedDeviceApiAdapter.h"

namespace ewfm {

class BinarySensorDeviceApiAdapter final
    : public TypedDeviceApiAdapter<BinarySensorDeviceApiAdapter, BinarySensorDevice, BinarySensorDeviceConfigV1> {
public:
    static constexpr const char* kTypeName = "binary_sensor";

    // Unlike ScheduleDevice (which never marks itself runtime-dirty), the sensor pushes every
    // debounced flip over WS, so exposing the live level here is accurate, not a stale snapshot.
    void writeRuntimeJson(const BinarySensorDevice& device, JsonObject runtimeJson) const {
        JsonObject outputJson = runtimeJson.createNestedObject("output");
        outputJson["active"] = device.isActive();
        outputJson["rawLevel"] = device.rawLevel();
        outputJson["hasReading"] = device.hasReading();
    }
};

} // namespace ewfm
