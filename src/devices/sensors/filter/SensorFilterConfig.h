#pragma once

#include "devices/core/DeviceTypes.h"

#include <ArduinoJson.h>
#include <cstdint>

namespace ewfm {

#pragma pack(push, 1)
struct SensorFilterConfigV1 {
    float smoothingWeight{1.0F};   // EMA weight: 1.0 disables smoothing (pass-through)
    float calibrationFactor{1.0F}; // linear scale: 1.0 disables calibration
    float calibrationOffset{0.0F}; // linear offset: 0.0 disables calibration

    bool parseJson(const JsonObjectConst& input, const char*& error);
    DeviceValidationResult validate() const;
    void writeJson(JsonObject output) const;
};
#pragma pack(pop)

} // namespace ewfm
